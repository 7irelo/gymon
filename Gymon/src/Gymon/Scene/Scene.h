#pragma once

#include "Gymon/Core.h"
#include "Gymon/Renderer/Material.h"
#include "Gymon/Renderer/Mesh.h"
#include "Gymon/Renderer/PerspectiveCamera.h"

#include <glm/glm.hpp>

#include <string>
#include <vector>

namespace Gymon {

	// Position/rotation/scale, kept as Euler angles in degrees because that is
	// what an inspector needs to show and edit. Converted to a matrix only when
	// something actually draws.
	struct TransformComponent
	{
		glm::vec3 Translation{ 0.0f, 0.0f, 0.0f };
		glm::vec3 Rotation{ 0.0f, 0.0f, 0.0f };
		glm::vec3 Scale{ 1.0f, 1.0f, 1.0f };

		glm::mat4 GetTransform() const;
	};

	// What kind of thing an entity is. Cameras and lights have no mesh but
	// still belong in the hierarchy, which is why this is not simply
	// "has a mesh or does not".
	enum class EntityType
	{
		Empty = 0,
		Mesh,
		Camera,
		DirectionalLight
	};

	// A named node in the scene.
	//
	// This is deliberately a plain struct rather than an ECS: the engine has one
	// scene of tens of objects, and a component registry would be more
	// machinery than the problem needs. Entities are held by Ref so that the
	// editor can keep a selection across insertions and removals.
	class Entity
	{
	public:
		Entity(uint32_t id, const std::string& name, EntityType type)
			: m_ID(id), Name(name), Type(type) {}

		uint32_t GetID() const { return m_ID; }

		std::string Name;
		EntityType Type = EntityType::Empty;
		TransformComponent Transform;

		bool Visible = true;

		// Only meaningful for EntityType::Mesh.
		Ref<Gymon::Mesh> Mesh;
		Ref<Gymon::Material> Material;

		// Only meaningful for EntityType::DirectionalLight.
		glm::vec3 LightColor{ 1.0f, 1.0f, 1.0f };
		float LightIntensity = 1.0f;

	private:
		uint32_t m_ID;
	};

	class Scene
	{
	public:
		explicit Scene(const std::string& name = "Scene") : m_Name(name) {}

		const std::string& GetName() const { return m_Name; }
		void SetName(const std::string& name) { m_Name = name; }

		Ref<Entity> CreateEntity(const std::string& name, EntityType type = EntityType::Empty);
		void DestroyEntity(uint32_t id);

		Ref<Entity> FindEntity(uint32_t id) const;

		const std::vector<Ref<Entity>>& GetEntities() const { return m_Entities; }

		// Draws every visible mesh entity. Lighting uniforms come from the
		// first directional light in the scene, if there is one.
		void OnRender(PerspectiveCamera& camera);

	private:
		std::string m_Name;
		std::vector<Ref<Entity>> m_Entities;

		// Monotonic, so an id is never reused and a stale selection resolves to
		// nothing rather than to a different entity.
		uint32_t m_NextID = 1;
	};
}
