#pragma once

#include "Gymon/Core.h"
#include "Gymon/Renderer/Mesh.h"

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

		// glTF pbrMetallicRoughness baseColorFactor. The engine's shading is
		// Blinn-Phong rather than PBR, so this is used as the albedo tint and
		// the metallic/roughness terms are ignored.
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
	// Geometry only: positions, normals, texture coordinates and indices, with
	// each node's world transform baked into its vertices so the result needs
	// no scene-graph support to draw correctly. Textures are not loaded yet;
	// materials come through as their base colour factor.
	//
	// Never throws. On failure the returned model has Success == false and an
	// Error describing why, so a bad asset cannot take the editor down.
	LoadedModel LoadModel(const std::string& filepath);
}
