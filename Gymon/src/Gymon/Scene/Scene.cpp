#include "gypch.h"
#include "Gymon/Scene/Scene.h"

#include "Gymon/Renderer/Renderer.h"

#include <glm/gtc/matrix_transform.hpp>

#include <limits>

namespace Gymon {

	glm::mat4 TransformComponent::GetTransform() const
	{
		glm::mat4 transform = glm::translate(glm::mat4(1.0f), Translation);

		// Y-X-Z, the order most editors present as yaw/pitch/roll.
		transform = glm::rotate(transform, glm::radians(Rotation.y), { 0.0f, 1.0f, 0.0f });
		transform = glm::rotate(transform, glm::radians(Rotation.x), { 1.0f, 0.0f, 0.0f });
		transform = glm::rotate(transform, glm::radians(Rotation.z), { 0.0f, 0.0f, 1.0f });

		return glm::scale(transform, Scale);
	}

	Ref<Entity> Scene::CreateEntity(const std::string& name, EntityType type)
	{
		auto entity = CreateRef<Entity>(m_NextID++, name, type);
		m_Entities.push_back(entity);
		return entity;
	}

	void Scene::DestroyEntity(uint32_t id)
	{
		m_Entities.erase(
			std::remove_if(m_Entities.begin(), m_Entities.end(),
				[id](const Ref<Entity>& e) { return e->GetID() == id; }),
			m_Entities.end());
	}

	Ref<Entity> Scene::FindEntity(uint32_t id) const
	{
		for (const auto& entity : m_Entities)
		{
			if (entity->GetID() == id)
				return entity;
		}
		return nullptr;
	}

	glm::vec3 Scene::GetLightDirection() const
	{
		for (const auto& entity : m_Entities)
		{
			if (entity->Type != EntityType::DirectionalLight || !entity->Visible)
				continue;

			// A directional light has no position, only an orientation, so its
			// rotation is what matters. -Z is forward.
			const glm::mat4 rotation = entity->Transform.GetTransform();
			return glm::normalize(glm::vec3(rotation * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f)));
		}

		return glm::normalize(glm::vec3(-0.5f, -1.0f, -0.3f));
	}

	void Scene::GetBounds(glm::vec3& center, float& radius) const
	{
		glm::vec3 min(std::numeric_limits<float>::max());
		glm::vec3 max(std::numeric_limits<float>::lowest());
		bool any = false;

		for (const auto& entity : m_Entities)
		{
			if (!entity->Visible || entity->Type != EntityType::Mesh || !entity->Mesh)
				continue;

			const glm::mat4 transform = entity->Transform.GetTransform();
			const glm::vec3 lo = entity->Mesh->GetBoundsMin();
			const glm::vec3 hi = entity->Mesh->GetBoundsMax();

			// Transform all eight corners rather than the two extremes: under
			// rotation the min/max corners do not stay the min/max.
			for (int i = 0; i < 8; i++)
			{
				const glm::vec3 corner(
					(i & 1) ? hi.x : lo.x,
					(i & 2) ? hi.y : lo.y,
					(i & 4) ? hi.z : lo.z);

				const glm::vec3 world = glm::vec3(transform * glm::vec4(corner, 1.0f));
				min = glm::min(min, world);
				max = glm::max(max, world);
				any = true;
			}
		}

		if (!any)
		{
			center = glm::vec3(0.0f);
			radius = 1.0f;
			return;
		}

		center = (min + max) * 0.5f;

		// Half the diagonal, floored so a single flat plane still gets a frustum
		// with some depth to it.
		radius = glm::max(glm::length(max - min) * 0.5f, 0.5f);
	}

	void Scene::OnRender(PerspectiveCamera& camera, const Ref<ShadowMap>& shadowMap,
		const Ref<Shader>& depthShader, uint32_t viewportWidth, uint32_t viewportHeight)
	{
		// One directional light drives the whole scene; the shaders take a
		// single light direction, so extra lights would be silently ignored.
		const glm::vec3 lightDirection = GetLightDirection();
		glm::vec3 lightColor{ 1.0f, 1.0f, 1.0f };

		for (const auto& entity : m_Entities)
		{
			if (entity->Type != EntityType::DirectionalLight || !entity->Visible)
				continue;

			lightColor = entity->LightColor * entity->LightIntensity;
			break;
		}

		const bool castShadows = shadowMap && depthShader && viewportWidth > 0 && viewportHeight > 0;

		if (castShadows)
		{
			glm::vec3 center;
			float radius;
			GetBounds(center, radius);

			// For a scene smaller than the shadow distance, cover all of it.
			// For a larger one, follow the camera: put the frustum a little
			// ahead of the eye, where the pixels that matter are.
			if (radius > m_Environment.ShadowDistance)
			{
				const glm::vec3 forward = glm::normalize(glm::vec3(
					camera.GetViewMatrix()[0][2],
					camera.GetViewMatrix()[1][2],
					camera.GetViewMatrix()[2][2]) * -1.0f);

				center = camera.GetPosition() + forward * (m_Environment.ShadowDistance * 0.55f);
				radius = m_Environment.ShadowDistance;
			}

			shadowMap->SetLight(lightDirection, center, radius);

			shadowMap->BeginPass();
			depthShader->Bind();
			depthShader->SetMat4("u_LightSpaceMatrix", shadowMap->GetLightSpaceMatrix());

			for (const auto& entity : m_Entities)
			{
				if (!entity->Visible || entity->Type != EntityType::Mesh || !entity->Mesh)
					continue;

				depthShader->SetMat4("u_Transform", entity->Transform.GetTransform());
				RenderCommand::DrawIndexed(entity->Mesh->GetVertexArray(), entity->Mesh->GetIndexCount());
			}

			shadowMap->EndPass(viewportWidth, viewportHeight);
		}

		Renderer::BeginScene(camera);

		for (const auto& entity : m_Entities)
		{
			if (!entity->Visible || entity->Type != EntityType::Mesh)
				continue;
			if (!entity->Mesh || !entity->Material)
				continue;

			auto& material = entity->Material;
			material->Set("u_LightDirection", lightDirection);
			material->Set("u_LightColor", lightColor);
			material->Set("u_ViewPosition", camera.GetPosition());
			material->Set("u_SkyColor", m_Environment.SkyColor);
			material->Set("u_HorizonColor", m_Environment.HorizonColor);
			material->Set("u_GroundColor", m_Environment.GroundColor);
			material->Set("u_AmbientIntensity", m_Environment.AmbientIntensity);
			material->Set("u_Exposure", m_Environment.Exposure);
			material->Set("u_HasShadowMap", castShadows ? 1 : 0);

			if (castShadows)
			{
				material->Set("u_LightSpaceMatrix", shadowMap->GetLightSpaceMatrix());
				// Unit 3; units 0-2 belong to the PBR maps.
				material->Set("u_ShadowMap", 3);
			}

			material->Bind();

			// Bound after the material so it cannot be clobbered by a material
			// that also happens to use unit 3.
			if (castShadows)
				shadowMap->Bind(3);

			Renderer::Submit(material->GetShader(), entity->Mesh->GetVertexArray(),
				entity->Transform.GetTransform(), entity->Mesh->GetIndexCount());
		}

		Renderer::EndScene();
	}
}
