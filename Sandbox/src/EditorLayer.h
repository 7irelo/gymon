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
	void DrawStatsPanel();
	void DrawViewport();

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
	glm::vec2 m_ViewportSize{ 0.0f, 0.0f };
	bool m_ViewportFocused = false;
	bool m_ViewportHovered = false;

	// Rolling frame timing for the stats panel. A single frame's delta is too
	// noisy to read, so it is smoothed.
	float m_FrameTimeMs = 0.0f;
	uint32_t m_ShaderReloads = 0;
};
