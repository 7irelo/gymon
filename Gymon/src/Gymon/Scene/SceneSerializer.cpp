#include "gypch.h"
#include "Gymon/Scene/SceneSerializer.h"

#include <json/json.hpp>

#include <fstream>

using json = nlohmann::json;

namespace Gymon {

	namespace {

		const char* TypeToString(EntityType type)
		{
			switch (type)
			{
				case EntityType::Empty:            return "empty";
				case EntityType::Mesh:             return "mesh";
				case EntityType::Camera:           return "camera";
				case EntityType::DirectionalLight: return "directional_light";
			}
			return "empty";
		}

		EntityType TypeFromString(const std::string& value)
		{
			if (value == "mesh")              return EntityType::Mesh;
			if (value == "camera")            return EntityType::Camera;
			if (value == "directional_light") return EntityType::DirectionalLight;
			return EntityType::Empty;
		}

		json ToJson(const glm::vec3& v) { return json::array({ v.x, v.y, v.z }); }
		json ToJson(const glm::vec4& v) { return json::array({ v.x, v.y, v.z, v.w }); }

		glm::vec3 Vec3From(const json& j, const glm::vec3& fallback)
		{
			if (!j.is_array() || j.size() != 3)
				return fallback;
			return { j[0].get<float>(), j[1].get<float>(), j[2].get<float>() };
		}

		glm::vec4 Vec4From(const json& j, const glm::vec4& fallback)
		{
			if (!j.is_array() || j.size() != 4)
				return fallback;
			return { j[0].get<float>(), j[1].get<float>(), j[2].get<float>(), j[3].get<float>() };
		}

		// Primitives are stored by name rather than by vertex data: a cube is
		// generated identically every run, and writing out its 24 vertices
		// would make scene files unreadable for no benefit.
		const char* PrimitiveName(const Ref<Mesh>& mesh)
		{
			return mesh ? mesh->GetPrimitiveName() : "none";
		}

		Ref<Mesh> PrimitiveFromName(const std::string& name)
		{
			if (name == "cube")   return Mesh::CreateCube();
			if (name == "plane")  return Mesh::CreatePlane();
			if (name == "sphere") return Mesh::CreateSphere();
			return nullptr;
		}
	}

	bool SceneSerializer::Serialize(const std::string& filepath) const
	{
		if (!m_Scene)
			return false;

		json root;
		root["version"] = 1;
		root["scene"] = m_Scene->GetName();

		json entities = json::array();
		for (const auto& entity : m_Scene->GetEntities())
		{
			json e;
			e["name"] = entity->Name;
			e["type"] = TypeToString(entity->Type);
			e["visible"] = entity->Visible;

			e["transform"] = {
				{ "translation", ToJson(entity->Transform.Translation) },
				{ "rotation",    ToJson(entity->Transform.Rotation) },
				{ "scale",       ToJson(entity->Transform.Scale) }
			};

			if (entity->Type == EntityType::Mesh)
			{
				e["mesh"] = PrimitiveName(entity->Mesh);

				if (entity->Material)
				{
					e["material"] = {
						{ "name",      entity->Material->GetName() },
						{ "shader",    entity->Material->GetShader() ? entity->Material->GetShader()->GetName() : "" },
						{ "albedo",    ToJson(entity->Material->Albedo) },
						{ "shininess", entity->Material->Shininess }
					};
				}
			}

			if (entity->Type == EntityType::DirectionalLight)
			{
				e["light"] = {
					{ "color",     ToJson(entity->LightColor) },
					{ "intensity", entity->LightIntensity }
				};
			}

			entities.push_back(e);
		}
		root["entities"] = entities;

		std::ofstream out(filepath);
		if (!out)
		{
			GY_CORE_ERROR("Could not open '{0}' for writing", filepath);
			return false;
		}

		out << root.dump(2) << '\n';
		GY_CORE_INFO("Saved scene to '{0}'", filepath);
		return true;
	}

	bool SceneSerializer::Deserialize(const std::string& filepath, ShaderLibrary& shaders)
	{
		std::ifstream in(filepath);
		if (!in)
		{
			GY_CORE_ERROR("Could not open scene '{0}'", filepath);
			return false;
		}

		json root;
		try
		{
			in >> root;
		}
		catch (const std::exception& e)
		{
			GY_CORE_ERROR("Could not parse scene '{0}': {1}", filepath, e.what());
			return false;
		}

		if (!root.contains("entities") || !root["entities"].is_array())
		{
			GY_CORE_ERROR("Scene '{0}' has no entities array", filepath);
			return false;
		}

		// Everything below this point succeeds, so the scene is only cleared
		// once the file is known to be usable.
		auto scene = CreateRef<Scene>(root.value("scene", std::string("Scene")));

		for (const auto& e : root["entities"])
		{
			const auto type = TypeFromString(e.value("type", std::string("empty")));
			auto entity = scene->CreateEntity(e.value("name", std::string("Entity")), type);
			entity->Visible = e.value("visible", true);

			if (e.contains("transform"))
			{
				const auto& t = e["transform"];
				entity->Transform.Translation = Vec3From(t.value("translation", json{}), { 0.0f, 0.0f, 0.0f });
				entity->Transform.Rotation = Vec3From(t.value("rotation", json{}), { 0.0f, 0.0f, 0.0f });
				entity->Transform.Scale = Vec3From(t.value("scale", json{}), { 1.0f, 1.0f, 1.0f });
			}

			if (type == EntityType::Mesh)
			{
				entity->Mesh = PrimitiveFromName(e.value("mesh", std::string("none")));

				if (e.contains("material"))
				{
					const auto& m = e["material"];
					const auto shaderName = m.value("shader", std::string(""));

					Ref<Shader> shader;
					if (!shaderName.empty() && shaders.Exists(shaderName))
						shader = shaders.Get(shaderName);
					else if (!shaderName.empty())
						GY_CORE_WARN("Scene references unknown shader '{0}'", shaderName);

					auto material = CreateRef<Material>(shader, m.value("name", std::string("Material")));
					material->Albedo = Vec4From(m.value("albedo", json{}), { 1.0f, 1.0f, 1.0f, 1.0f });
					material->Shininess = m.value("shininess", 32.0f);
					entity->Material = material;
				}
			}

			if (type == EntityType::DirectionalLight && e.contains("light"))
			{
				const auto& l = e["light"];
				entity->LightColor = Vec3From(l.value("color", json{}), { 1.0f, 1.0f, 1.0f });
				entity->LightIntensity = l.value("intensity", 1.0f);
			}
		}

		*m_Scene = *scene;
		GY_CORE_INFO("Loaded scene '{0}' ({1} entities)", filepath, m_Scene->GetEntities().size());
		return true;
	}
}
