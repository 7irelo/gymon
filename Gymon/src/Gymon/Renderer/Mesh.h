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

		// "cube", "plane", "sphere", or "custom" for meshes built from raw
		// vertex data. Scene files store this rather than the vertices: a
		// primitive regenerates identically, and writing out its vertices
		// would make scene files unreadable for no benefit.
		const char* GetPrimitiveName() const { return m_PrimitiveName; }
		void SetPrimitiveName(const char* name) { m_PrimitiveName = name; }

		// Local-space axis-aligned bounds, computed once at construction.
		// Needed to fit a shadow frustum around the scene: without real bounds
		// the light's orthographic box has to be guessed, and a guess that is
		// too small clips shadows while one that is too large wastes texels.
		const glm::vec3& GetBoundsMin() const { return m_BoundsMin; }
		const glm::vec3& GetBoundsMax() const { return m_BoundsMax; }

		// Built-in primitives
		static Ref<Mesh> CreateCube();
		static Ref<Mesh> CreatePlane();
		// UV sphere. Defaults give a smooth-enough ball without a silly vertex
		// count; raise the segment counts for close-ups.
		static Ref<Mesh> CreateSphere(uint32_t latitudeSegments = 24, uint32_t longitudeSegments = 48);
	private:
		Ref<VertexArray> m_VertexArray;
		uint32_t m_IndexCount;
		const char* m_PrimitiveName = "custom";

		glm::vec3 m_BoundsMin{ 0.0f };
		glm::vec3 m_BoundsMax{ 0.0f };
	};
}
