#pragma once

#include "Gymon/Core.h"
#include "Gymon/Renderer/Shader.h"
#include "Gymon/Renderer/Texture.h"

#include <glm/glm.hpp>

#include <string>
#include <unordered_map>
#include <variant>

namespace Gymon {

	// A shader plus the uniform values to draw with it.
	//
	// Meshes describe geometry and materials describe appearance, so the same
	// cube can be drawn red and shiny in one entity and matte grey in another
	// without duplicating either the mesh or the shader.
	//
	// Uniforms are stored in a small typed map and uploaded on Bind(), which
	// means the editor can edit them by name without the renderer knowing what
	// any particular shader expects.
	class Material
	{
	public:
		using Value = std::variant<int, float, glm::vec2, glm::vec3, glm::vec4, glm::mat4>;

		Material(const Ref<Shader>& shader, const std::string& name = "Material");

		const std::string& GetName() const { return m_Name; }
		void SetName(const std::string& name) { m_Name = name; }

		const Ref<Shader>& GetShader() const { return m_Shader; }
		void SetShader(const Ref<Shader>& shader) { m_Shader = shader; }

		void Set(const std::string& name, const Value& value) { m_Uniforms[name] = value; }
		bool Has(const std::string& name) const { return m_Uniforms.find(name) != m_Uniforms.end(); }

		// Returns nullptr when the uniform has not been set, so callers can
		// distinguish "unset" from "set to zero".
		const Value* Get(const std::string& name) const;

		const std::unordered_map<std::string, Value>& GetUniforms() const { return m_Uniforms; }

		// Bound to texture unit 0 and exposed to the shader as u_Texture.
		void SetTexture(const Ref<Texture2D>& texture) { m_Texture = texture; }
		const Ref<Texture2D>& GetTexture() const { return m_Texture; }

		// Common properties, surfaced directly so the inspector has something
		// meaningful to show for every material regardless of its shader.
		glm::vec4 Albedo{ 1.0f, 1.0f, 1.0f, 1.0f };
		float Shininess = 32.0f;

		// Binds the shader, uploads Albedo/Shininess and every stored uniform,
		// and binds the texture if there is one.
		void Bind() const;

	private:
		std::string m_Name;
		Ref<Shader> m_Shader;
		Ref<Texture2D> m_Texture;
		std::unordered_map<std::string, Value> m_Uniforms;
	};
}
