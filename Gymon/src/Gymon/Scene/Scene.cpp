#include "gypch.h"
#include "Gymon/Scene/Scene.h"

#include "Gymon/Renderer/Renderer.h"

#include <glm/gtc/matrix_transform.hpp>

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

	void Scene::OnRender(PerspectiveCamera& camera)
	{
		// One directional light drives the whole scene; the shaders take a
		// single light direction, so extra lights would be silently ignored.
		glm::vec3 lightDirection{ -0.5f, -1.0f, -0.3f };
		glm::vec3 lightColor{ 1.0f, 1.0f, 1.0f };

		for (const auto& entity : m_Entities)
		{
			if (entity->Type != EntityType::DirectionalLight || !entity->Visible)
				continue;

			// A directional light has no position, only an orientation, so its
			// rotation is what matters. -Z is forward.
			const glm::mat4 rotation = entity->Transform.GetTransform();
			lightDirection = glm::normalize(glm::vec3(rotation * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f)));
			lightColor = entity->LightColor * entity->LightIntensity;
			break;
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
			material->Bind();

			Renderer::Submit(material->GetShader(), entity->Mesh->GetVertexArray(),
				entity->Transform.GetTransform(), entity->Mesh->GetIndexCount());
		}

		Renderer::EndScene();
	}
}
