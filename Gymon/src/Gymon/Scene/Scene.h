#pragma once

#include "Gymon/Core.h"
#include "Gymon/Renderer/Material.h"
#include "Gymon/Renderer/Mesh.h"
#include "Gymon/Renderer/PerspectiveCamera.h"
#include "Gymon/Renderer/ShadowMap.h"

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

	// Everything about the scene that is not an entity: the sky the ambient
	// term is derived from, and the exposure the tonemapper uses.
	//
	// Kept on the Scene rather than on each material because it describes the
	// world, not a surface -- and because the sky pass and the surface shading
	// must agree on it or lit objects will not sit in their background.
	struct Environment
	{
		glm::vec3 SkyColor{ 0.20f, 0.36f, 0.68f };
		glm::vec3 HorizonColor{ 0.62f, 0.71f, 0.84f };
		glm::vec3 GroundColor{ 0.16f, 0.14f, 0.12f };

		float AmbientIntensity = 0.30f;
		float Exposure = 1.0f;

		// Radius, in world units, of the region around the camera the shadow
		// map covers. Fitting the map to the whole scene instead sounds
		// tidier, but a two-hundred-metre circuit spread over 2048 texels puts
		// a shadow texel at seven centimetres, and every contact shadow turns
		// to mush. Following the camera keeps the detail where it is looked
		// at, at the cost of shadows fading out in the far distance.
		float ShadowDistance = 55.0f;
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

		Environment& GetEnvironment() { return m_Environment; }
		const Environment& GetEnvironment() const { return m_Environment; }

		// Draws every visible mesh entity. Lighting uniforms come from the
		// first directional light in the scene, if there is one.
		//
		// When a shadow map and depth shader are supplied, a depth pass runs
		// first and the resulting map is bound for the main pass.
		void OnRender(PerspectiveCamera& camera,
			const Ref<ShadowMap>& shadowMap = nullptr,
			const Ref<Shader>& depthShader = nullptr,
			uint32_t viewportWidth = 0, uint32_t viewportHeight = 0);

		// World-space bounding sphere of everything visible, used to fit the
		// shadow frustum. Returns a unit sphere at the origin for an empty
		// scene so callers never have to special-case that.
		void GetBounds(glm::vec3& center, float& radius) const;

		// Direction the first enabled directional light points, or a sensible
		// default when the scene has none.
		glm::vec3 GetLightDirection() const;

	private:
		std::string m_Name;
		std::vector<Ref<Entity>> m_Entities;
		Environment m_Environment;

		// Monotonic, so an id is never reused and a stale selection resolves to
		// nothing rather than to a different entity.
		uint32_t m_NextID = 1;
	};
}
