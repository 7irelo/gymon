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

		// The two properties every material has, whether or not the shader
		// declares them. Setting a uniform a shader does not use is a no-op in
		// GL, so this stays safe across shaders.
		m_Shader->SetFloat4("u_Color", Albedo);
		m_Shader->SetFloat("u_Shininess", Shininess);

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

		if (m_Texture)
		{
			m_Texture->Bind(0);
			m_Shader->SetInt("u_Texture", 0);
		}
	}
}
