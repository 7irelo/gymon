#pragma once

#include <Gymon.h>

// The editor demo: a scene of entities, a hierarchy panel, an inspector, and a
// live statistics window. Shader hot-reload is wired in here too, so editing
// assets/shaders/Lit.glsl while this layer is active updates the scene.
class EditorLayer : public Gymon::Layer
{
public:
	EditorLayer();
	virtual ~EditorLayer() = default;

	virtual void OnAttach() override;
	virtual void OnDetach() override;

	virtual void OnUpdate(Gymon::Timestep ts) override;
	virtual void OnImGuiRender() override;
	virtual void OnEvent(Gymon::Event& e) override;

	void SetActive(bool active) { m_Active = active; }
	bool IsActive() const { return m_Active; }

private:
	void BuildScene();

	// Fullscreen passes framing the scene: the sky is drawn first with the
	// depth test off, the grid last with depth testing on but depth writes
	// off, so geometry occludes it without it occluding anything.
	void DrawSky();
	void DrawGrid();

	void DrawStatsPanel();
	void DrawViewport();
	void DrawMenuBar();

	// Hosts every other panel in a fullscreen dockspace and, on first run,
	// splits it into the standard editor layout.
	void DrawDockspace();
	void BuildDefaultLayout(unsigned int dockspaceID);

	// Gizmo mode buttons and display toggles, in a strip under the menu bar.
	void DrawToolbar();

	void SaveScene();
	void LoadScene();

	// Loads a glTF file and adds one entity per primitive, so a multi-part
	// model stays selectable and editable part by part.
	void ImportModel(const std::string& path);

private:
	bool m_Active = false;

	Gymon::PerspectiveCameraController m_CameraController;

	Gymon::Ref<Gymon::Scene> m_Scene;
	Gymon::SceneHierarchyPanel m_Hierarchy;
	Gymon::PropertiesPanel m_Inspector;

	Gymon::ShaderLibrary m_Shaders;

	// The scene renders into this and is displayed as an ImGui image, so it
	// sits inside a viewport panel instead of behind the whole UI.
	Gymon::Ref<Gymon::Framebuffer> m_Framebuffer;

	// Re-rendered every frame. The scene is small enough that caching it
	// against an unchanged light and unchanged transforms would be more
	// bookkeeping than the pass costs.
	Gymon::Ref<Gymon::ShadowMap> m_ShadowMap;

	// Editor display options, surfaced in the toolbar.
	// Off by default now that the demo scene has real ground under it: the
	// grid helps place things in an empty scene, and over a textured surface
	// it is a lattice of lines in the way.
	bool m_ShowGrid = false;
	bool m_ShowSky = true;
	float m_Exposure = 1.0f;

	// The default split is built once, then imgui.ini takes over so a layout
	// the user rearranged survives a restart.
	bool m_LayoutBuilt = false;
	bool m_ResetLayout = false;
	glm::vec2 m_ViewportSize{ 0.0f, 0.0f };
	bool m_ViewportFocused = false;
	bool m_ViewportHovered = false;

	// ImGuizmo operation, as its enum value. Stored as an int so the header
	// does not have to pull ImGuizmo in; -1 means no gizmo.
	int m_GizmoOperation = -1;

	// Rolling frame timing for the stats panel. A single frame's delta is too
	// noisy to read, so it is smoothed.
	float m_FrameTimeMs = 0.0f;
	uint32_t m_ShaderReloads = 0;
};
