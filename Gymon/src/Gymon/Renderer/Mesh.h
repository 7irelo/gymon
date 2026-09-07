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
		// UV sphere. Defaults give a smooth-enough ball without a silly vertex
		// count; raise the segment counts for close-ups.
		static Ref<Mesh> CreateSphere(uint32_t latitudeSegments = 24, uint32_t longitudeSegments = 48);
	private:
		Ref<VertexArray> m_VertexArray;
		uint32_t m_IndexCount;
	};
}
