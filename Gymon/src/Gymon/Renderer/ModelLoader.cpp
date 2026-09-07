#include "gypch.h"
#include "Gymon/Renderer/ModelLoader.h"

// tinygltf decodes images with stb_image, and so does the engine. Including
// stb_image.h here and telling tinygltf not to include it again means both
// share the single implementation in stb_image.cpp, instead of tinygltf
// emitting a second one and failing to link on duplicate symbols.
#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_INCLUDE_STB_IMAGE
#include <stb_image.h>
#include <tinygltf/tiny_gltf.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>

#include <filesystem>

namespace Gymon {

	namespace {

		// Reads component `index` of accessor `accessor` as a float, coping
		// with the byte strides glTF allows and with the integer index types.
		const unsigned char* AccessorData(const tinygltf::Model& model, const tinygltf::Accessor& accessor, size_t& strideOut)
		{
			const auto& view = model.bufferViews[accessor.bufferView];
			const auto& buffer = model.buffers[view.buffer];

			const int componentSize = tinygltf::GetComponentSizeInBytes(accessor.componentType);
			const int componentCount = tinygltf::GetNumComponentsInType(accessor.type);

			// A stride of 0 means tightly packed.
			strideOut = view.byteStride ? view.byteStride : static_cast<size_t>(componentSize) * componentCount;

			return buffer.data.data() + view.byteOffset + accessor.byteOffset;
		}

		glm::mat4 NodeLocalTransform(const tinygltf::Node& node)
		{
			// glTF nodes carry either a full matrix or a TRS triple, never both.
			if (node.matrix.size() == 16)
			{
				glm::mat4 m(1.0f);
				for (int col = 0; col < 4; col++)
					for (int row = 0; row < 4; row++)
						m[col][row] = static_cast<float>(node.matrix[col * 4 + row]);
				return m;
			}

			glm::mat4 transform(1.0f);

			if (node.translation.size() == 3)
			{
				transform = glm::translate(transform, glm::vec3(
					static_cast<float>(node.translation[0]),
					static_cast<float>(node.translation[1]),
					static_cast<float>(node.translation[2])));
			}

			if (node.rotation.size() == 4)
			{
				// glTF stores quaternions xyzw; glm's constructor takes wxyz.
				const glm::quat q(
					static_cast<float>(node.rotation[3]),
					static_cast<float>(node.rotation[0]),
					static_cast<float>(node.rotation[1]),
					static_cast<float>(node.rotation[2]));
				transform *= glm::mat4_cast(q);
			}

			if (node.scale.size() == 3)
			{
				transform = glm::scale(transform, glm::vec3(
					static_cast<float>(node.scale[0]),
					static_cast<float>(node.scale[1]),
					static_cast<float>(node.scale[2])));
			}

			return transform;
		}

		// Uploads a decoded glTF image. tinygltf has already turned the source
		// (external file, data URI, or a chunk inside a .glb) into raw pixels,
		// so this only has to get them onto the GPU.
		//
		// sRGB matters here: albedo maps are authored in sRGB and must be
		// linearised before lighting, while normal and metallic-roughness maps
		// hold raw numbers and must not be.
		Ref<Texture2D> UploadImage(const tinygltf::Model& model, int textureIndex, bool srgb)
		{
			if (textureIndex < 0 || textureIndex >= static_cast<int>(model.textures.size()))
				return nullptr;

			const int source = model.textures[textureIndex].source;
			if (source < 0 || source >= static_cast<int>(model.images.size()))
				return nullptr;

			const auto& image = model.images[source];
			if (image.width <= 0 || image.height <= 0 || image.image.empty())
				return nullptr;

			TextureSpecification spec;
			spec.MinFilter = TextureFilter::Linear;
			spec.MagFilter = TextureFilter::Linear;
			spec.GenerateMips = true;   // model textures are minified constantly
			spec.SRGB = srgb;

			auto texture = Texture2D::Create(
				static_cast<uint32_t>(image.width), static_cast<uint32_t>(image.height), spec);

			// tinygltf gives 8-bit RGB or RGBA. The engine's texture storage is
			// RGBA8, so a 3-component image is expanded rather than uploaded
			// with a mismatched row length.
			if (image.component == 4)
			{
				texture->SetData(const_cast<unsigned char*>(image.image.data()),
					static_cast<uint32_t>(image.image.size()));
			}
			else if (image.component == 3)
			{
				std::vector<unsigned char> rgba(static_cast<size_t>(image.width) * image.height * 4);
				for (size_t i = 0, n = static_cast<size_t>(image.width) * image.height; i < n; i++)
				{
					rgba[i * 4 + 0] = image.image[i * 3 + 0];
					rgba[i * 4 + 1] = image.image[i * 3 + 1];
					rgba[i * 4 + 2] = image.image[i * 3 + 2];
					rgba[i * 4 + 3] = 255;
				}
				texture->SetData(rgba.data(), static_cast<uint32_t>(rgba.size()));
			}
			else
			{
				GY_CORE_WARN("Unsupported glTF image with {0} components", image.component);
				return nullptr;
			}

			return texture;
		}

		void LoadPrimitive(const tinygltf::Model& model, const tinygltf::Primitive& primitive,
			const glm::mat4& worldTransform, const std::string& name, LoadedModel& out)
		{
			// Only triangles. Points, lines and strips would need a different
			// draw mode than the renderer exposes.
			if (primitive.mode != TINYGLTF_MODE_TRIANGLES)
				return;

			const auto positionIt = primitive.attributes.find("POSITION");
			if (positionIt == primitive.attributes.end())
				return;

			const auto& posAccessor = model.accessors[positionIt->second];
			size_t posStride = 0;
			const unsigned char* posData = AccessorData(model, posAccessor, posStride);

			const unsigned char* normalData = nullptr;
			size_t normalStride = 0;
			if (auto it = primitive.attributes.find("NORMAL"); it != primitive.attributes.end())
				normalData = AccessorData(model, model.accessors[it->second], normalStride);

			const unsigned char* uvData = nullptr;
			size_t uvStride = 0;
			if (auto it = primitive.attributes.find("TEXCOORD_0"); it != primitive.attributes.end())
				uvData = AccessorData(model, model.accessors[it->second], uvStride);

			// Normals are transformed by the inverse-transpose so that a
			// non-uniformly scaled node still lights correctly.
			const glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(worldTransform)));

			std::vector<Vertex> vertices;
			vertices.reserve(posAccessor.count);

			for (size_t i = 0; i < posAccessor.count; i++)
			{
				Vertex vertex;

				const float* p = reinterpret_cast<const float*>(posData + i * posStride);
				vertex.Position = glm::vec3(worldTransform * glm::vec4(p[0], p[1], p[2], 1.0f));

				if (normalData)
				{
					const float* n = reinterpret_cast<const float*>(normalData + i * normalStride);
					vertex.Normal = glm::normalize(normalMatrix * glm::vec3(n[0], n[1], n[2]));
				}
				else
				{
					// glTF says normals may be omitted, in which case flat
					// normals are implied. Up is a serviceable placeholder.
					vertex.Normal = { 0.0f, 1.0f, 0.0f };
				}

				if (uvData)
				{
					const float* uv = reinterpret_cast<const float*>(uvData + i * uvStride);
					vertex.TexCoord = { uv[0], uv[1] };
				}
				else
				{
					vertex.TexCoord = { 0.0f, 0.0f };
				}

				vertices.push_back(vertex);
			}

			std::vector<uint32_t> indices;
			if (primitive.indices >= 0)
			{
				const auto& indexAccessor = model.accessors[primitive.indices];
				size_t indexStride = 0;
				const unsigned char* indexData = AccessorData(model, indexAccessor, indexStride);

				indices.reserve(indexAccessor.count);
				for (size_t i = 0; i < indexAccessor.count; i++)
				{
					const unsigned char* element = indexData + i * indexStride;
					switch (indexAccessor.componentType)
					{
						case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
							indices.push_back(*reinterpret_cast<const uint8_t*>(element));
							break;
						case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
							indices.push_back(*reinterpret_cast<const uint16_t*>(element));
							break;
						case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
							indices.push_back(*reinterpret_cast<const uint32_t*>(element));
							break;
						default:
							return;
					}
				}
			}
			else
			{
				// Non-indexed geometry: synthesise a trivial index buffer, since
				// the renderer only draws indexed.
				indices.resize(vertices.size());
				for (uint32_t i = 0; i < indices.size(); i++)
					indices[i] = i;
			}

			if (vertices.empty() || indices.empty())
				return;

			LoadedPrimitive loaded;
			loaded.Name = name;
			loaded.Mesh = CreateRef<Mesh>(vertices, indices);

			if (primitive.material >= 0 && primitive.material < static_cast<int>(model.materials.size()))
			{
				const auto& material = model.materials[primitive.material];
				const auto& pbr = material.pbrMetallicRoughness;

				if (pbr.baseColorFactor.size() == 4)
				{
					loaded.BaseColor = {
						static_cast<float>(pbr.baseColorFactor[0]), static_cast<float>(pbr.baseColorFactor[1]),
						static_cast<float>(pbr.baseColorFactor[2]), static_cast<float>(pbr.baseColorFactor[3])
					};
				}

				loaded.Metallic = static_cast<float>(pbr.metallicFactor);
				loaded.Roughness = static_cast<float>(pbr.roughnessFactor);

				loaded.AlbedoMap = UploadImage(model, pbr.baseColorTexture.index, true);
				loaded.MetallicRoughnessMap = UploadImage(model, pbr.metallicRoughnessTexture.index, false);
				loaded.NormalMap = UploadImage(model, material.normalTexture.index, false);
			}

			out.Primitives.push_back(std::move(loaded));
		}

		void LoadNode(const tinygltf::Model& model, int nodeIndex, const glm::mat4& parentTransform, LoadedModel& out)
		{
			if (nodeIndex < 0 || nodeIndex >= static_cast<int>(model.nodes.size()))
				return;

			const auto& node = model.nodes[nodeIndex];
			const glm::mat4 world = parentTransform * NodeLocalTransform(node);

			if (node.mesh >= 0 && node.mesh < static_cast<int>(model.meshes.size()))
			{
				const auto& mesh = model.meshes[node.mesh];
				const std::string baseName = !node.name.empty() ? node.name
					: (!mesh.name.empty() ? mesh.name : "Mesh");

				for (size_t i = 0; i < mesh.primitives.size(); i++)
				{
					const std::string name = mesh.primitives.size() == 1
						? baseName
						: baseName + "." + std::to_string(i);
					LoadPrimitive(model, mesh.primitives[i], world, name, out);
				}
			}

			for (int child : node.children)
				LoadNode(model, child, world, out);
		}
	}

	LoadedModel LoadModel(const std::string& filepath)
	{
		LoadedModel result;

		tinygltf::Model model;
		tinygltf::TinyGLTF loader;
		std::string err, warn;

		const std::string extension = std::filesystem::path(filepath).extension().string();
		const bool binary = extension == ".glb" || extension == ".GLB";

		bool ok = false;
		try
		{
			ok = binary
				? loader.LoadBinaryFromFile(&model, &err, &warn, filepath)
				: loader.LoadASCIIFromFile(&model, &err, &warn, filepath);
		}
		catch (const std::exception& e)
		{
			result.Error = e.what();
			GY_CORE_ERROR("Failed to load model '{0}': {1}", filepath, result.Error);
			return result;
		}

		if (!warn.empty())
			GY_CORE_WARN("glTF warning for '{0}': {1}", filepath, warn);

		if (!ok)
		{
			result.Error = err.empty() ? "unknown glTF error" : err;
			GY_CORE_ERROR("Failed to load model '{0}': {1}", filepath, result.Error);
			return result;
		}

		// Walk the default scene when there is one; otherwise every root node,
		// since a file is allowed to omit the scene list.
		if (!model.scenes.empty())
		{
			const int sceneIndex = model.defaultScene >= 0 ? model.defaultScene : 0;
			for (int node : model.scenes[sceneIndex].nodes)
				LoadNode(model, node, glm::mat4(1.0f), result);
		}
		else
		{
			for (size_t i = 0; i < model.nodes.size(); i++)
				LoadNode(model, static_cast<int>(i), glm::mat4(1.0f), result);
		}

		if (result.Primitives.empty())
		{
			result.Error = "no triangle geometry found";
			GY_CORE_ERROR("Model '{0}' contained no drawable geometry", filepath);
			return result;
		}

		result.Success = true;
		GY_CORE_INFO("Loaded model '{0}' ({1} primitive{2})",
			filepath, result.Primitives.size(), result.Primitives.size() == 1 ? "" : "s");
		return result;
	}
}
