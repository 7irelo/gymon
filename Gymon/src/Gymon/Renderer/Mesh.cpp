#include "gypch.h"
#include "Gymon/Renderer/Mesh.h"

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
