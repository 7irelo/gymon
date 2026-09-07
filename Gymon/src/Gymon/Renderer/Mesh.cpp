#include "gypch.h"
#include "Gymon/Renderer/Mesh.h"

#include <cmath>

namespace Gymon {

	Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
		: m_IndexCount((uint32_t)indices.size())
	{
		if (!vertices.empty())
		{
			m_BoundsMin = m_BoundsMax = vertices[0].Position;
			for (const auto& v : vertices)
			{
				m_BoundsMin = glm::min(m_BoundsMin, v.Position);
				m_BoundsMax = glm::max(m_BoundsMax, v.Position);
			}
		}

		m_VertexArray = VertexArray::Create();

		Ref<VertexBuffer> vertexBuffer = VertexBuffer::Create(
			(float*)vertices.data(), (uint32_t)(vertices.size() * sizeof(Vertex)));
		vertexBuffer->SetLayout({
			{ ShaderDataType::Float3, "a_Position" },
			{ ShaderDataType::Float3, "a_Normal"   },
			{ ShaderDataType::Float2, "a_TexCoord" }
		});
		m_VertexArray->AddVertexBuffer(vertexBuffer);

		Ref<IndexBuffer> indexBuffer = IndexBuffer::Create(
			const_cast<uint32_t*>(indices.data()), (uint32_t)indices.size());
		m_VertexArray->SetIndexBuffer(indexBuffer);
	}

	Ref<Mesh> Mesh::CreateCube()
	{
		// 24 vertices (4 per face) so each face gets its own normal & UVs.
		std::vector<Vertex> vertices = {
			// Front (+Z)
			{ { -0.5f, -0.5f,  0.5f }, {  0.0f,  0.0f,  1.0f }, { 0.0f, 0.0f } },
			{ {  0.5f, -0.5f,  0.5f }, {  0.0f,  0.0f,  1.0f }, { 1.0f, 0.0f } },
			{ {  0.5f,  0.5f,  0.5f }, {  0.0f,  0.0f,  1.0f }, { 1.0f, 1.0f } },
			{ { -0.5f,  0.5f,  0.5f }, {  0.0f,  0.0f,  1.0f }, { 0.0f, 1.0f } },
			// Back (-Z)
			{ {  0.5f, -0.5f, -0.5f }, {  0.0f,  0.0f, -1.0f }, { 0.0f, 0.0f } },
			{ { -0.5f, -0.5f, -0.5f }, {  0.0f,  0.0f, -1.0f }, { 1.0f, 0.0f } },
			{ { -0.5f,  0.5f, -0.5f }, {  0.0f,  0.0f, -1.0f }, { 1.0f, 1.0f } },
			{ {  0.5f,  0.5f, -0.5f }, {  0.0f,  0.0f, -1.0f }, { 0.0f, 1.0f } },
			// Left (-X)
			{ { -0.5f, -0.5f, -0.5f }, { -1.0f,  0.0f,  0.0f }, { 0.0f, 0.0f } },
			{ { -0.5f, -0.5f,  0.5f }, { -1.0f,  0.0f,  0.0f }, { 1.0f, 0.0f } },
			{ { -0.5f,  0.5f,  0.5f }, { -1.0f,  0.0f,  0.0f }, { 1.0f, 1.0f } },
			{ { -0.5f,  0.5f, -0.5f }, { -1.0f,  0.0f,  0.0f }, { 0.0f, 1.0f } },
			// Right (+X)
			{ {  0.5f, -0.5f,  0.5f }, {  1.0f,  0.0f,  0.0f }, { 0.0f, 0.0f } },
			{ {  0.5f, -0.5f, -0.5f }, {  1.0f,  0.0f,  0.0f }, { 1.0f, 0.0f } },
			{ {  0.5f,  0.5f, -0.5f }, {  1.0f,  0.0f,  0.0f }, { 1.0f, 1.0f } },
			{ {  0.5f,  0.5f,  0.5f }, {  1.0f,  0.0f,  0.0f }, { 0.0f, 1.0f } },
			// Top (+Y)
			{ { -0.5f,  0.5f,  0.5f }, {  0.0f,  1.0f,  0.0f }, { 0.0f, 0.0f } },
			{ {  0.5f,  0.5f,  0.5f }, {  0.0f,  1.0f,  0.0f }, { 1.0f, 0.0f } },
			{ {  0.5f,  0.5f, -0.5f }, {  0.0f,  1.0f,  0.0f }, { 1.0f, 1.0f } },
			{ { -0.5f,  0.5f, -0.5f }, {  0.0f,  1.0f,  0.0f }, { 0.0f, 1.0f } },
			// Bottom (-Y)
			{ { -0.5f, -0.5f, -0.5f }, {  0.0f, -1.0f,  0.0f }, { 0.0f, 0.0f } },
			{ {  0.5f, -0.5f, -0.5f }, {  0.0f, -1.0f,  0.0f }, { 1.0f, 0.0f } },
			{ {  0.5f, -0.5f,  0.5f }, {  0.0f, -1.0f,  0.0f }, { 1.0f, 1.0f } },
			{ { -0.5f, -0.5f,  0.5f }, {  0.0f, -1.0f,  0.0f }, { 0.0f, 1.0f } }
		};

		std::vector<uint32_t> indices;
		indices.reserve(36);
		for (uint32_t face = 0; face < 6; face++)
		{
			uint32_t offset = face * 4;
			indices.push_back(offset + 0);
			indices.push_back(offset + 1);
			indices.push_back(offset + 2);
			indices.push_back(offset + 2);
			indices.push_back(offset + 3);
			indices.push_back(offset + 0);
		}

		auto mesh = CreateRef<Mesh>(vertices, indices);
		mesh->SetPrimitiveName("cube");
		return mesh;
	}

	Ref<Mesh> Mesh::CreatePlane()
	{
		std::vector<Vertex> vertices = {
			{ { -0.5f, 0.0f,  0.5f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f } },
			{ {  0.5f, 0.0f,  0.5f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f } },
			{ {  0.5f, 0.0f, -0.5f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 1.0f } },
			{ { -0.5f, 0.0f, -0.5f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f } }
		};
		std::vector<uint32_t> indices = { 0, 1, 2, 2, 3, 0 };

		auto mesh = CreateRef<Mesh>(vertices, indices);
		mesh->SetPrimitiveName("plane");
		return mesh;
	}

	Ref<Mesh> Mesh::CreateCylinder(uint32_t segments)
	{
		segments = segments < 3 ? 3 : segments;

		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;

		const float halfHeight = 0.5f;
		const float radius = 0.5f;
		const float twoPi = 6.28318530718f;

		// The side wall and the caps do not share vertices: a shared vertex
		// would have to pick one normal, and the whole point of a cylinder is
		// that the rim is a hard edge.
		const uint32_t sideStart = 0;
		for (uint32_t i = 0; i <= segments; i++)
		{
			const float t = (float)i / (float)segments;
			const float angle = t * twoPi;
			const float x = std::cos(angle), z = std::sin(angle);
			const glm::vec3 normal(x, 0.0f, z);

			vertices.push_back({ { x * radius, -halfHeight, z * radius }, normal, { t, 0.0f } });
			vertices.push_back({ { x * radius,  halfHeight, z * radius }, normal, { t, 1.0f } });
		}

		for (uint32_t i = 0; i < segments; i++)
		{
			const uint32_t base = sideStart + i * 2;
			indices.insert(indices.end(), { base, base + 2, base + 3 });
			indices.insert(indices.end(), { base, base + 3, base + 1 });
		}

		// Caps, as triangle fans around a centre vertex.
		for (int side = 0; side < 2; side++)
		{
			const float y = side == 0 ? halfHeight : -halfHeight;
			const glm::vec3 normal(0.0f, side == 0 ? 1.0f : -1.0f, 0.0f);

			const uint32_t centre = (uint32_t)vertices.size();
			vertices.push_back({ { 0.0f, y, 0.0f }, normal, { 0.5f, 0.5f } });

			for (uint32_t i = 0; i <= segments; i++)
			{
				const float angle = (float)i / (float)segments * twoPi;
				const float x = std::cos(angle), z = std::sin(angle);
				vertices.push_back({ { x * radius, y, z * radius }, normal,
					{ x * 0.5f + 0.5f, z * 0.5f + 0.5f } });
			}

			for (uint32_t i = 0; i < segments; i++)
			{
				const uint32_t a = centre + 1 + i;
				const uint32_t b = centre + 2 + i;
				// Wind the bottom cap the other way round so both faces point
				// outwards.
				if (side == 0)
					indices.insert(indices.end(), { centre, a, b });
				else
					indices.insert(indices.end(), { centre, b, a });
			}
		}

		auto mesh = CreateRef<Mesh>(vertices, indices);
		mesh->SetPrimitiveName("cylinder");
		return mesh;
	}

	Ref<Mesh> Mesh::CreateSphere(uint32_t latitudeSegments, uint32_t longitudeSegments)
	{
		// Standard UV sphere: walk latitude bands from pole to pole, and for
		// each band walk longitude all the way round. The seam vertex is
		// duplicated (lon runs to <= longitudeSegments) so texture coordinates
		// wrap cleanly instead of interpolating back across the whole texture.
		latitudeSegments = latitudeSegments < 2 ? 2 : latitudeSegments;
		longitudeSegments = longitudeSegments < 3 ? 3 : longitudeSegments;

		constexpr float pi = 3.14159265358979323846f;
		const float radius = 0.5f;

		std::vector<Vertex> vertices;
		vertices.reserve((latitudeSegments + 1) * (longitudeSegments + 1));

		for (uint32_t lat = 0; lat <= latitudeSegments; lat++)
		{
			const float v = static_cast<float>(lat) / static_cast<float>(latitudeSegments);
			const float theta = v * pi;
			const float sinTheta = std::sin(theta);
			const float cosTheta = std::cos(theta);

			for (uint32_t lon = 0; lon <= longitudeSegments; lon++)
			{
				const float u = static_cast<float>(lon) / static_cast<float>(longitudeSegments);
				const float phi = u * 2.0f * pi;

				const glm::vec3 normal{ sinTheta * std::cos(phi), cosTheta, sinTheta * std::sin(phi) };

				Vertex vertex;
				vertex.Position = normal * radius;
				vertex.Normal = normal;   // unit sphere: the normal is the direction
				vertex.TexCoord = { u, 1.0f - v };
				vertices.push_back(vertex);
			}
		}

		std::vector<uint32_t> indices;
		indices.reserve(latitudeSegments * longitudeSegments * 6);

		const uint32_t stride = longitudeSegments + 1;
		for (uint32_t lat = 0; lat < latitudeSegments; lat++)
		{
			for (uint32_t lon = 0; lon < longitudeSegments; lon++)
			{
				const uint32_t first = lat * stride + lon;
				const uint32_t second = first + stride;

				indices.push_back(first);
				indices.push_back(second);
				indices.push_back(first + 1);

				indices.push_back(second);
				indices.push_back(second + 1);
				indices.push_back(first + 1);
			}
		}

		auto mesh = CreateRef<Mesh>(vertices, indices);
		mesh->SetPrimitiveName("sphere");
		return mesh;
	}
}
