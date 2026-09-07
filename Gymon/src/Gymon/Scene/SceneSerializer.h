#pragma once

#include "Gymon/Core.h"
#include "Gymon/Scene/Scene.h"

#include <string>

namespace Gymon {

	// Reads and writes scenes as JSON.
	//
	// Without this a scene only exists as C++ in a BuildScene() function, which
	// means every change to a level is a recompile. A scene file makes levels
	// data, so they can be authored in the editor and loaded at runtime.
	//
	// Materials reference their shader by name, which is resolved against a
	// ShaderLibrary on load: shader source belongs in .glsl files, not embedded
	// in every scene that happens to use it.
	class SceneSerializer
	{
	public:
		explicit SceneSerializer(const Ref<Scene>& scene) : m_Scene(scene) {}

		bool Serialize(const std::string& filepath) const;

		// Replaces the scene's contents. Returns false and leaves the scene
		// untouched if the file is missing or malformed, so a bad load does not
		// destroy what is already open.
		bool Deserialize(const std::string& filepath, ShaderLibrary& shaders);

	private:
		Ref<Scene> m_Scene;
	};
}
