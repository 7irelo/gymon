#pragma once

#include "Gymon/Core.h"
#include "Gymon/Scene/Scene.h"

namespace Gymon {

	// The two halves of the editor UI: a list of what is in the scene, and the
	// properties of whatever is selected in it. They share a selection, which
	// the hierarchy owns and the inspector reads.
	class SceneHierarchyPanel
	{
	public:
		SceneHierarchyPanel() = default;
		explicit SceneHierarchyPanel(const Ref<Scene>& scene) { SetContext(scene); }

		void SetContext(const Ref<Scene>& scene);

		const Ref<Scene>& GetContext() const { return m_Scene; }

		Ref<Entity> GetSelected() const;
		void SetSelected(const Ref<Entity>& entity);

		void OnImGuiRender();

	private:
		void DrawEntityNode(const Ref<Entity>& entity);

	private:
		Ref<Scene> m_Scene;

		// Held by id rather than by Ref so that deleting an entity elsewhere
		// leaves the selection resolving to nothing instead of keeping the
		// entity alive behind the scene's back.
		uint32_t m_SelectedID = 0;
	};

	class PropertiesPanel
	{
	public:
		void OnImGuiRender(const Ref<Scene>& scene, const Ref<Entity>& entity);
	};
}
