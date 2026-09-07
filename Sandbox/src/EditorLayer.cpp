#include "EditorLayer.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <ImGuizmo.h>

#include <filesystem>

#include <glm/gtc/type_ptr.hpp>

EditorLayer::EditorLayer()
	: Layer("EditorLayer"), m_CameraController(1280.0f / 720.0f, 45.0f)
{
}

void EditorLayer::OnAttach()
{
	// Loaded from disk on purpose: a file-backed shader is what
	// ShaderLibrary::ReloadChanged() can watch and recompile.
	m_Shaders.Load("assets/shaders/Lit.glsl");
	m_Shaders.Load("assets/shaders/PBR.glsl");
	m_Shaders.Load("assets/shaders/ShadowDepth.glsl");
	m_Shaders.Load("assets/shaders/Sky.glsl");
	m_Shaders.Load("assets/shaders/Grid.glsl");

	m_ShadowMap = Gymon::CreateRef<Gymon::ShadowMap>(2048);

	Gymon::FramebufferSpecification fbSpec;
	fbSpec.Width = 1280;
	fbSpec.Height = 720;
	m_Framebuffer = Gymon::Framebuffer::Create(fbSpec);

	BuildScene();
	m_Hierarchy.SetContext(m_Scene);

	// Set through the controller: it owns the position and rewrites the
	// camera's every update, so setting the camera directly would not stick.
	m_CameraController.SetYawPitch(-108.0f, -14.0f);
	m_CameraController.SetPosition({ 4.5f, 3.2f, 8.5f });
}

void EditorLayer::BuildScene()
{
	m_Scene = Gymon::CreateRef<Gymon::Scene>("Scene");

	auto lit = m_Shaders.Get("PBR");

	auto camera = m_Scene->CreateEntity("Camera", Gymon::EntityType::Camera);
	camera->Transform.Translation = { 3.0f, 2.5f, 6.0f };

	auto light = m_Scene->CreateEntity("Directional Light", Gymon::EntityType::DirectionalLight);
	light->Transform.Rotation = { -45.0f, -30.0f, 0.0f };
	light->LightColor = { 1.0f, 0.96f, 0.9f };

	auto cube = m_Scene->CreateEntity("Cube", Gymon::EntityType::Mesh);
	cube->Mesh = Gymon::Mesh::CreateCube();
	cube->Material = Gymon::CreateRef<Gymon::Material>(lit, "Cube Material");
	cube->Material->Albedo = { 0.85f, 0.35f, 0.30f, 1.0f };
	cube->Material->Roughness = 0.55f;
	cube->Transform.Translation = { -1.4f, 0.5f, 0.0f };

	auto sphere = m_Scene->CreateEntity("Sphere", Gymon::EntityType::Mesh);
	sphere->Mesh = Gymon::Mesh::CreateSphere();
	sphere->Material = Gymon::CreateRef<Gymon::Material>(lit, "Sphere Material");
	sphere->Material->Albedo = { 0.94f, 0.78f, 0.35f, 1.0f };
	// Polished gold: metals tint their reflection with albedo and have no
	// diffuse term, which is the clearest demonstration of the BRDF.
	sphere->Material->Metallic = 1.0f;
	sphere->Material->Roughness = 0.18f;
	sphere->Transform.Translation = { 1.4f, 0.5f, 0.0f };

	auto plane = m_Scene->CreateEntity("Plane", Gymon::EntityType::Mesh);
	plane->Mesh = Gymon::Mesh::CreatePlane();
	plane->Material = Gymon::CreateRef<Gymon::Material>(lit, "Ground Material");
	plane->Material->Albedo = { 0.42f, 0.44f, 0.47f, 1.0f };
	plane->Material->Roughness = 0.85f;
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

	if (m_ShowSky)
		DrawSky();

	const auto& renderSpec = m_Framebuffer->GetSpecification();
	m_Scene->OnRender(m_CameraController.GetCamera(), m_ShadowMap,
		m_Shaders.Get("ShadowDepth"), renderSpec.Width, renderSpec.Height);

	if (m_ShowGrid)
		DrawGrid();

	m_Framebuffer->Unbind();
}

void EditorLayer::DrawSky()
{
	auto shader = m_Shaders.Get("Sky");
	if (!shader)
		return;

	const auto& camera = m_CameraController.GetCamera();

	// Depth writes off as well as the test: the sky must not fill the depth
	// buffer, or every subsequent draw would be rejected.
	Gymon::RenderCommand::SetDepthTest(false);
	Gymon::RenderCommand::SetDepthWrite(false);

	shader->Bind();
	shader->SetMat4("u_InverseViewProjection", glm::inverse(camera.GetViewProjectionMatrix()));
	shader->SetFloat3("u_CameraPosition", camera.GetPosition());
	shader->SetFloat3("u_LightDirection", m_Scene->GetLightDirection());
	shader->SetFloat3("u_LightColor", glm::vec3(1.0f, 0.96f, 0.9f));

	const auto& environment = m_Scene->GetEnvironment();
	shader->SetFloat3("u_SkyColor", environment.SkyColor);
	shader->SetFloat3("u_HorizonColor", environment.HorizonColor);
	shader->SetFloat3("u_GroundColor", environment.GroundColor);
	shader->SetFloat("u_Exposure", environment.Exposure);

	Gymon::RenderCommand::DrawArrays(3);

	Gymon::RenderCommand::SetDepthWrite(true);
	Gymon::RenderCommand::SetDepthTest(true);
}

void EditorLayer::DrawGrid()
{
	auto shader = m_Shaders.Get("Grid");
	if (!shader)
		return;

	const auto& camera = m_CameraController.GetCamera();
	const glm::mat4 viewProjection = camera.GetViewProjectionMatrix();

	// Blended, and writing no depth: the grid is an overlay on the world, not
	// part of it, so it must not occlude anything drawn after it.
	Gymon::RenderCommand::SetBlend(true);
	Gymon::RenderCommand::SetDepthWrite(false);

	shader->Bind();
	shader->SetMat4("u_ViewProjection", viewProjection);
	shader->SetMat4("u_InverseViewProjection", glm::inverse(viewProjection));
	shader->SetFloat3("u_CameraPosition", camera.GetPosition());

	Gymon::RenderCommand::DrawArrays(3);

	Gymon::RenderCommand::SetDepthWrite(true);
	Gymon::RenderCommand::SetBlend(false);
}

void EditorLayer::DrawStatsPanel()
{
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

	auto lit = m_Shaders.Get("PBR");

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
		if (ImGui::BeginMenu("Gizmo"))
		{
			if (ImGui::MenuItem("None", "Q", m_GizmoOperation == -1))
				m_GizmoOperation = -1;
			if (ImGui::MenuItem("Translate", "W", m_GizmoOperation == (int)ImGuizmo::TRANSLATE))
				m_GizmoOperation = (int)ImGuizmo::TRANSLATE;
			if (ImGui::MenuItem("Rotate", "E", m_GizmoOperation == (int)ImGuizmo::ROTATE))
				m_GizmoOperation = (int)ImGuizmo::ROTATE;
			if (ImGui::MenuItem("Scale", "R", m_GizmoOperation == (int)ImGuizmo::SCALE))
				m_GizmoOperation = (int)ImGuizmo::SCALE;
			ImGui::EndMenu();
		}

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

		if (ImGui::BeginMenu("View"))
		{
			ImGui::MenuItem("Grid", nullptr, &m_ShowGrid);
			ImGui::MenuItem("Sky", nullptr, &m_ShowSky);
			ImGui::Separator();
			if (ImGui::MenuItem("Reset Layout"))
				m_ResetLayout = true;
			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}
}

void EditorLayer::DrawViewport()
{
	// No padding: the image should meet the panel edges like a real viewport.
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("Viewport");

	m_ViewportFocused = ImGui::IsWindowFocused();
	m_ViewportHovered = ImGui::IsWindowHovered();

	const ImVec2 viewportMin = ImGui::GetWindowPos();
	const ImVec2 viewportOffset = ImGui::GetCursorPos();

	const ImVec2 available = ImGui::GetContentRegionAvail();
	m_ViewportSize = { available.x, available.y };

	// GL textures have their origin bottom-left, so the UVs are flipped
	// vertically to present the image the right way up.
	ImGui::Image(
		(ImTextureID)(uintptr_t)m_Framebuffer->GetColorAttachmentRendererID(),
		available,
		ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));

	// Gizmo, drawn over the viewport image.
	auto selected = m_Hierarchy.GetSelected();
	if (selected && m_GizmoOperation >= 0)
	{
		ImGuizmo::SetOrthographic(false);
		ImGuizmo::SetDrawlist();
		ImGuizmo::SetRect(
			viewportMin.x + viewportOffset.x, viewportMin.y + viewportOffset.y,
			available.x, available.y);

		const auto& camera = m_CameraController.GetCamera();
		const glm::mat4 view = camera.GetViewMatrix();
		const glm::mat4 projection = camera.GetProjectionMatrix();

		glm::mat4 transform = selected->Transform.GetTransform();

		ImGuizmo::Manipulate(
			glm::value_ptr(view), glm::value_ptr(projection),
			(ImGuizmo::OPERATION)m_GizmoOperation, ImGuizmo::LOCAL,
			glm::value_ptr(transform));

		if (ImGuizmo::IsUsing())
		{
			// Decompose straight back into the same translation/euler/scale
			// the inspector edits, so dragging a handle and typing a number
			// stay in agreement.
			float translation[3], rotation[3], scale[3];
			ImGuizmo::DecomposeMatrixToComponents(
				glm::value_ptr(transform), translation, rotation, scale);

			selected->Transform.Translation = { translation[0], translation[1], translation[2] };
			selected->Transform.Rotation = { rotation[0], rotation[1], rotation[2] };
			selected->Transform.Scale = { scale[0], scale[1], scale[2] };
		}
	}

	ImGui::End();
	ImGui::PopStyleVar();
}

void EditorLayer::OnImGuiRender()
{
	if (!m_Active)
		return;

	// ImGuizmo piggybacks on the ImGui frame, so it has to be told a new one
	// started before anything tries to draw a gizmo.
	ImGuizmo::BeginFrame();

	// Opens the host window, draws the menu bar and toolbar into it, and
	// declares the dockspace every other panel docks into.
	DrawDockspace();

	m_Hierarchy.OnImGuiRender();
	m_Inspector.OnImGuiRender(m_Scene, m_Hierarchy.GetSelected());
	DrawStatsPanel();
	DrawViewport();

	// Ends the dockspace host window opened by DrawDockspace.
	ImGui::End();
}

void EditorLayer::DrawDockspace()
{
	// A borderless, immovable window covering the whole main viewport. Every
	// panel docks into it, which is what turns a set of floating windows into
	// an editor.
	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);
	ImGui::SetNextWindowViewport(viewport->ID);

	ImGuiWindowFlags flags =
		ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
		ImGuiWindowFlags_NoNavFocus;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("##Dockspace", nullptr, flags);
	ImGui::PopStyleVar(3);

	DrawMenuBar();
	DrawToolbar();

	const ImGuiID dockspaceID = ImGui::GetID("GymonDockspace");

	// Only lay panels out when there is nothing to restore. Once imgui.ini
	// has a layout, that is the user's arrangement, and overwriting it on
	// every launch would be hostile.
	if (!m_LayoutBuilt || m_ResetLayout)
	{
		if (m_ResetLayout || ImGui::DockBuilderGetNode(dockspaceID) == nullptr)
			BuildDefaultLayout(dockspaceID);

		m_LayoutBuilt = true;
		m_ResetLayout = false;
	}

	ImGui::DockSpace(dockspaceID, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
}

void EditorLayer::BuildDefaultLayout(unsigned int dockspaceID)
{
	const ImGuiViewport* viewport = ImGui::GetMainViewport();

	ImGui::DockBuilderRemoveNode(dockspaceID);
	ImGui::DockBuilderAddNode(dockspaceID, ImGuiDockNodeFlags_DockSpace);
	ImGui::DockBuilderSetNodeSize(dockspaceID, viewport->WorkSize);

	// Split off the sides first and the bottom last, so the outer columns run
	// the full height of the window the way they do in Unreal and Unity.
	ImGuiID centre = dockspaceID;
	const ImGuiID left = ImGui::DockBuilderSplitNode(centre, ImGuiDir_Left, 0.19f, nullptr, &centre);
	const ImGuiID right = ImGui::DockBuilderSplitNode(centre, ImGuiDir_Right, 0.24f, nullptr, &centre);

	// Both halves are captured: splitting turns `left` into a parent node, so
	// docking a window into it afterwards would silently do nothing.
	ImGuiID leftTop = left;
	const ImGuiID leftBottom = ImGui::DockBuilderSplitNode(left, ImGuiDir_Down, 0.42f, nullptr, &leftTop);

	ImGui::DockBuilderDockWindow("Scene Hierarchy", leftTop);
	ImGui::DockBuilderDockWindow("Statistics", leftBottom);
	ImGui::DockBuilderDockWindow("Inspector", right);
	ImGui::DockBuilderDockWindow("Viewport", centre);

	ImGui::DockBuilderFinish(dockspaceID);
}

void EditorLayer::DrawToolbar()
{
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6.0f, 4.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 5.0f));
	ImGui::Dummy(ImVec2(0.0f, 1.0f));
	ImGui::Indent(8.0f);

	struct Tool { const char* label; int operation; const char* tip; };
	const Tool tools[] = {
		{ "Select",    -1,                         "Q - no gizmo" },
		{ "Move",      (int)ImGuizmo::TRANSLATE,   "W - translate" },
		{ "Rotate",    (int)ImGuizmo::ROTATE,      "E - rotate" },
		{ "Scale",     (int)ImGuizmo::SCALE,       "R - scale" }
	};

	for (int i = 0; i < IM_ARRAYSIZE(tools); i++)
	{
		if (i > 0)
			ImGui::SameLine();

		const bool active = m_GizmoOperation == tools[i].operation;
		if (active)
			ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));

		if (ImGui::Button(tools[i].label))
			m_GizmoOperation = tools[i].operation;

		if (active)
			ImGui::PopStyleColor();

		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("%s", tools[i].tip);
	}

	ImGui::SameLine();
	ImGui::TextUnformatted("|");
	ImGui::SameLine();
	ImGui::Checkbox("Grid", &m_ShowGrid);
	ImGui::SameLine();
	ImGui::Checkbox("Sky", &m_ShowSky);

	ImGui::SameLine();
	ImGui::SetNextItemWidth(120.0f);
	if (ImGui::SliderFloat("Exposure", &m_Exposure, 0.2f, 3.0f, "%.2f"))
		m_Scene->GetEnvironment().Exposure = m_Exposure;

	ImGui::Unindent(8.0f);
	ImGui::Dummy(ImVec2(0.0f, 2.0f));
	ImGui::Separator();
	ImGui::PopStyleVar(2);
}

void EditorLayer::OnEvent(Gymon::Event& e)
{
	if (!m_Active)
		return;

	m_CameraController.OnEvent(e);

	Gymon::EventDispatcher dispatcher(e);
	dispatcher.Dispatch<Gymon::KeyPressedEvent>([this](Gymon::KeyPressedEvent& event)
	{
		// Gizmo shortcuts, only while the viewport has focus and nothing is
		// mid-drag, so typing in an inspector field cannot switch tools.
		if (!m_ViewportFocused || ImGuizmo::IsUsing() || event.IsRepeat())
			return false;

		switch (event.GetKeyCode())
		{
			case Gymon::Key::Q: m_GizmoOperation = -1; return true;
			case Gymon::Key::W: m_GizmoOperation = (int)ImGuizmo::TRANSLATE; return true;
			case Gymon::Key::E: m_GizmoOperation = (int)ImGuizmo::ROTATE; return true;
			case Gymon::Key::R: m_GizmoOperation = (int)ImGuizmo::SCALE; return true;
			default: return false;
		}
	});
}



