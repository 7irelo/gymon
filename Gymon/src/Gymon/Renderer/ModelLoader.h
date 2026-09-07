#pragma once

#include "Gymon/Core.h"
#include "Gymon/Renderer/Mesh.h"
#include "Gymon/Renderer/Texture.h"

#include <glm/glm.hpp>

#include <string>
#include <vector>

namespace Gymon {

	// One drawable piece of a loaded file. glTF splits a mesh into primitives,
	// each with its own material, so a single file usually produces several of
	// these rather than one mesh.
	struct LoadedPrimitive
	{
		std::string Name;
		Ref<Mesh> Mesh;

		// glTF pbrMetallicRoughness maps. Null when the material does not
		// supply one, in which case the shader falls back to the scalar
		// factors below.
		Ref<Texture2D> AlbedoMap;
		Ref<Texture2D> NormalMap;
		Ref<Texture2D> MetallicRoughnessMap;

		float Metallic = 0.0f;
		float Roughness = 0.9f;

		// glTF pbrMetallicRoughness baseColorFactor, used as an albedo tint and
		// multiplied with AlbedoMap when both are present.
		glm::vec4 BaseColor{ 1.0f, 1.0f, 1.0f, 1.0f };
	};

	struct LoadedModel
	{
		bool Success = false;
		std::string Error;
		std::vector<LoadedPrimitive> Primitives;
	};

	// Loads a .gltf or .glb file.
	//
	// Positions, normals, texture coordinates and indices, with each node's
	// world transform baked into its vertices so the result needs no
	// scene-graph support to draw correctly. Base colour, normal and
	// metallic-roughness textures are decoded and uploaded, including images
	// embedded in a .glb.
	//
	// Never throws. On failure the returned model has Success == false and an
	// Error describing why, so a bad asset cannot take the editor down.
	LoadedModel LoadModel(const std::string& filepath);
}
