#pragma once

#include "Gymon/Core.h"
#include "Gymon/Renderer/VertexArray.h"

#include <glm/glm.hpp>
#include <vector>

namespace Gymon {

	struct Vertex
	{
		glm::vec3 Position;
		glm::vec3 Normal;
		glm::vec2 TexCoord;
	};

	// A renderable indexed triangle mesh (positions, normals, texcoords).
	class Mesh
	{
	public:
		Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);

		const Ref<VertexArray>& GetVertexArray() const { return m_VertexArray; }

		uint32_t GetIndexCount() const { return m_IndexCount; }

		// Built-in primitives
		static Ref<Mesh> CreateCube();
		static Ref<Mesh> CreatePlane();
	private:
		Ref<VertexArray> m_VertexArray;
		uint32_t m_IndexCount;
	};
}
