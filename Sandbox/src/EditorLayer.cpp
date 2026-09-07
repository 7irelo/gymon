#include "EditorLayer.h"

#include <imgui.h>

#include <filesystem>

EditorLayer::EditorLayer()
	: Layer("EditorLayer"), m_CameraController(1280.0f / 720.0f, 45.0f)
{
}

void EditorLayer::OnAttach()
{
	// Loaded from disk on purpose: a file-backed shader is what
	// ShaderLibrary::ReloadChanged() can watch and recompile.
	m_Shaders.Load("assets/shaders/Lit.glsl");

	Gymon::FramebufferSpecification fbSpec;
	fbSpec.Width = 1280;
	fbSpec.Height = 720;
	m_Framebuffer = Gymon::Framebuffer::Create(fbSpec);

	BuildScene();
	m_Hierarchy.SetContext(m_Scene);

	m_CameraController.GetCamera().SetPosition({ 0.0f, 3.5f, 9.0f });
}

void EditorLayer::BuildScene()
{
	m_Scene = Gymon::CreateRef<Gymon::Scene>("Scene");

	auto lit = m_Shaders.Get("Lit");

	auto camera = m_Scene->CreateEntity("Camera", Gymon::EntityType::Camera);
	camera->Transform.Translation = { 3.0f, 2.5f, 6.0f };

	auto light = m_Scene->CreateEntity("Directional Light", Gymon::EntityType::DirectionalLight);
	light->Transform.Rotation = { -45.0f, -30.0f, 0.0f };
	light->LightColor = { 1.0f, 0.96f, 0.9f };

	auto cube = m_Scene->CreateEntity("Cube", Gymon::EntityType::Mesh);
	cube->Mesh = Gymon::Mesh::CreateCube();
	cube->Material = Gymon::CreateRef<Gymon::Material>(lit, "Cube Material");
	cube->Material->Albedo = { 0.85f, 0.35f, 0.30f, 1.0f };
	cube->Transform.Translation = { -1.4f, 0.5f, 0.0f };

	auto sphere = m_Scene->CreateEntity("Sphere", Gymon::EntityType::Mesh);
	sphere->Mesh = Gymon::Mesh::CreateSphere();
	sphere->Material = Gymon::CreateRef<Gymon::Material>(lit, "Sphere Material");
	sphere->Material->Albedo = { 0.35f, 0.55f, 0.9f, 1.0f };
	sphere->Material->Shininess = 96.0f;
	sphere->Transform.Translation = { 1.4f, 0.5f, 0.0f };

	auto plane = m_Scene->CreateEntity("Plane", Gymon::EntityType::Mesh);
	plane->Mesh = Gymon::Mesh::CreatePlane();
	plane->Material = Gymon::CreateRef<Gymon::Material>(lit, "Ground Material");
	plane->Material->Albedo = { 0.55f, 0.55f, 0.58f, 1.0f };
	plane->Material->Shininess = 8.0f;
	plane->Transform.Scale = { 12.0f, 1.0f, 12.0f };
}

void EditorLayer::OnDetach()
{
}

void EditorLayer::OnUpdate(Gymon::Timestep ts)
{
	if (!m_Active)
		return;

	// Exponential moving average; a raw per-frame delta is unreadable.
	const float frameMs = ts.GetMilliseconds();
	m_FrameTimeMs = m_FrameTimeMs == 0.0f ? frameMs : m_FrameTimeMs * 0.9f + frameMs * 0.1f;

	// Only stats the shader files unless one actually changed on disk.
	m_ShaderReloads += m_Shaders.ReloadChanged();

	// Resize before rendering, so the first frame at a new size is already
	// correct rather than a frame of stretched image.
	const auto& spec = m_Framebuffer->GetSpecification();
	if (m_ViewportSize.x > 0.0f && m_ViewportSize.y > 0.0f &&
		(spec.Width != (uint32_t)m_ViewportSize.x || spec.Height != (uint32_t)m_ViewportSize.y))
	{
		m_Framebuffer->Resize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
		m_CameraController.OnResize(m_ViewportSize.x, m_ViewportSize.y);
	}

	// Camera input only while the viewport has focus, otherwise WASD typed
	// into an inspector field would fly the camera around.
	if (m_ViewportFocused)
		m_CameraController.OnUpdate(ts);

	m_Framebuffer->Bind();

	Gymon::RenderCommand::SetClearColor({ 0.09f, 0.10f, 0.13f, 1.0f });
	Gymon::RenderCommand::Clear();

	Gymon::Renderer::ResetStats();
	m_Scene->OnRender(m_CameraController.GetCamera());

	m_Framebuffer->Unbind();
}

void EditorLayer::DrawStatsPanel()
{
	ImGui::SetNextWindowPos(ImVec2(20.0f, 380.0f), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(260.0f, 240.0f), ImGuiCond_FirstUseEver);
	ImGui::Begin("Statistics");

	const auto stats = Gymon::Renderer::GetStats();
	auto& window = Gymon::Application::Get().GetWindow();

	ImGui::Text("FPS:         %.0f", m_FrameTimeMs > 0.0f ? 1000.0f / m_FrameTimeMs : 0.0f);
	ImGui::Text("Frame time:  %.2f ms", m_FrameTimeMs);
	ImGui::Separator();
	ImGui::Text("Draw calls:  %u", stats.DrawCalls);
	ImGui::Text("Meshes:      %u", stats.MeshCount);
	ImGui::Text("Triangles:   %u", stats.TriangleCount);
	ImGui::Text("Vertices:    %u", stats.VertexCount);
	ImGui::Separator();
	ImGui::Text("Entities:    %zu", m_Scene->GetEntities().size());
	ImGui::Text("Camera:      Perspective");
	ImGui::Text("VSync:       %s", window.IsVSync() ? "On" : "Off");
	ImGui::Text("Shader reloads: %u", m_ShaderReloads);

	ImGui::End();
}

// Scenes are written next to the executable, alongside the assets the scene
// references, so a saved scene and its shaders travel together.
static const char* s_ScenePath = "assets/scenes/Editor.gyscene";

void EditorLayer::SaveScene()
{
	std::filesystem::create_directories("assets/scenes");
	Gymon::SceneSerializer(m_Scene).Serialize(s_ScenePath);
}

void EditorLayer::LoadScene()
{
	Gymon::SceneSerializer serializer(m_Scene);
	if (serializer.Deserialize(s_ScenePath, m_Shaders))
	{
		// The hierarchy holds a selection id that may no longer exist.
		m_Hierarchy.SetContext(m_Scene);
	}
}

void EditorLayer::ImportModel(const std::string& path)
{
	const auto model = Gymon::LoadModel(path);
	if (!model.Success)
		return;

	auto lit = m_Shaders.Get("Lit");

	for (const auto& primitive : model.Primitives)
	{
		auto entity = m_Scene->CreateEntity(primitive.Name, Gymon::EntityType::Mesh);
		entity->Mesh = primitive.Mesh;
		entity->Material = Gymon::CreateRef<Gymon::Material>(lit, primitive.Name + " Material");
		entity->Material->Albedo = primitive.BaseColor;
	}
}

void EditorLayer::DrawMenuBar()
{
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("Scene"))
		{
			if (ImGui::MenuItem("Save Scene", "Ctrl+S"))
				SaveScene();
			if (ImGui::MenuItem("Load Scene", "Ctrl+O"))
				LoadScene();
			ImGui::Separator();
			if (ImGui::MenuItem("Import glTF Model..."))
				ImportModel("assets/models/TestScene.gltf");
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}
}

void EditorLayer::DrawViewport()
{
	// No padding: the image should meet the panel edges like a real viewport.
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::SetNextWindowPos(ImVec2(300.0f, 40.0f), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(660.0f, 460.0f), ImGuiCond_FirstUseEver);
	ImGui::Begin("Viewport");

	m_ViewportFocused = ImGui::IsWindowFocused();
	m_ViewportHovered = ImGui::IsWindowHovered();

	const ImVec2 available = ImGui::GetContentRegionAvail();
	m_ViewportSize = { available.x, available.y };

	// GL textures have their origin bottom-left, so the UVs are flipped
	// vertically to present the image the right way up.
	ImGui::Image(
		(ImTextureID)(uintptr_t)m_Framebuffer->GetColorAttachmentRendererID(),
		available,
		ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));

	ImGui::End();
	ImGui::PopStyleVar();
}

void EditorLayer::OnImGuiRender()
{
	if (!m_Active)
		return;

	DrawMenuBar();
	m_Hierarchy.OnImGuiRender();
	m_Inspector.OnImGuiRender(m_Scene, m_Hierarchy.GetSelected());
	DrawStatsPanel();
	DrawViewport();
}

void EditorLayer::OnEvent(Gymon::Event& e)
{
	if (!m_Active)
		return;

	m_CameraController.OnEvent(e);
}


