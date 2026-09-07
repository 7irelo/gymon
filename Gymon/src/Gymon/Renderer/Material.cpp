#include "gypch.h"
#include "Gymon/Renderer/Material.h"

namespace Gymon {

	Material::Material(const Ref<Shader>& shader, const std::string& name)
		: m_Name(name), m_Shader(shader)
	{
	}

	const Material::Value* Material::Get(const std::string& name) const
	{
		auto it = m_Uniforms.find(name);
		return it == m_Uniforms.end() ? nullptr : &it->second;
	}

	void Material::Bind() const
	{
		if (!m_Shader)
			return;

		m_Shader->Bind();

		// The properties every material has, whether or not a given shader
		// declares them. Setting a uniform a shader does not use is a no-op in
		// GL, so this stays safe across shaders.
		//
		// Every one of these is uploaded unconditionally. Uniforms live on the
		// program, not on the material, so anything left unset here would keep
		// the value the previously bound material uploaded.
		m_Shader->SetFloat4("u_Albedo", Albedo);
		m_Shader->SetFloat("u_Metallic", Metallic);
		m_Shader->SetFloat("u_Roughness", Roughness);
		m_Shader->SetFloat2("u_Tiling", Tiling);

		for (const auto& [name, value] : m_Uniforms)
		{
			std::visit([&](const auto& v)
			{
				using T = std::decay_t<decltype(v)>;
				if constexpr (std::is_same_v<T, int>)
					m_Shader->SetInt(name, v);
				else if constexpr (std::is_same_v<T, float>)
					m_Shader->SetFloat(name, v);
				else if constexpr (std::is_same_v<T, glm::vec2>)
					m_Shader->SetFloat2(name, v);
				else if constexpr (std::is_same_v<T, glm::vec3>)
					m_Shader->SetFloat3(name, v);
				else if constexpr (std::is_same_v<T, glm::vec4>)
					m_Shader->SetFloat4(name, v);
				else if constexpr (std::is_same_v<T, glm::mat4>)
					m_Shader->SetMat4(name, v);
			}, value);
		}

		// Fixed sampler units, with a presence flag each so the shader can skip
		// a fetch rather than sampling an unbound unit (which reads black and
		// would make every untextured object render black).
		BindMap(AlbedoMap, 0, "u_AlbedoMap", "u_HasAlbedoMap");
		BindMap(NormalMap, 1, "u_NormalMap", "u_HasNormalMap");
		BindMap(MetallicRoughnessMap, 2, "u_MetallicRoughnessMap", "u_HasMetallicRoughnessMap");
	}

	void Material::BindMap(const Ref<Texture2D>& map, uint32_t slot,
		const char* samplerName, const char* presenceName) const
	{
		if (map)
		{
			map->Bind(slot);
			m_Shader->SetInt(samplerName, static_cast<int>(slot));
			m_Shader->SetInt(presenceName, 1);
		}
		else
		{
			m_Shader->SetInt(presenceName, 0);
		}
	}
}
