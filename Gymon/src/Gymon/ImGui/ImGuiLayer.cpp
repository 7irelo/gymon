#include "gypch.h"
#include "Gymon/ImGui/ImGuiLayer.h"

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include "Gymon/Application.h"

#include <filesystem>

// TEMPORARY
#include <GLFW/glfw3.h>
#include <glad/glad.h>

namespace Gymon {

	ImGuiLayer::ImGuiLayer()
		: Layer("ImGuiLayer")
	{
	}

	void ImGuiLayer::OnAttach()
	{
		// Setup Dear ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

		ImGui::StyleColorsDark();
		LoadFonts();
		SetDarkThemeColors();
		SetStyle();

		Application& app = Application::Get();
		GLFWwindow* window = static_cast<GLFWwindow*>(app.GetWindow().GetNativeWindow());

		ImGui_ImplGlfw_InitForOpenGL(window, true);
		ImGui_ImplOpenGL3_Init("#version 410");
	}

	void ImGuiLayer::OnDetach()
	{
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}

	void ImGuiLayer::OnEvent(Event& e)
	{
		if (m_BlockEvents)
		{
			ImGuiIO& io = ImGui::GetIO();
			e.Handled |= e.IsInCategory(EventCategoryMouse) & io.WantCaptureMouse;
			e.Handled |= e.IsInCategory(EventCategoryKeyboard) & io.WantCaptureKeyboard;
		}
	}

	void ImGuiLayer::Begin()
	{
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
	}

	void ImGuiLayer::End()
	{
		ImGuiIO& io = ImGui::GetIO();
		Application& app = Application::Get();
		io.DisplaySize = ImVec2((float)app.GetWindow().GetWidth(), (float)app.GetWindow().GetHeight());

		// Rendering
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	}

	namespace {

		// Candidate UI fonts, best first. A bundled font wins so a project can
		// ship its own look; otherwise a platform font is used. Nothing is
		// redistributed by the engine, which keeps font licensing out of it.
		const char* s_UIFontCandidates[] = {
			"assets/fonts/Inter-Regular.ttf",
			"assets/fonts/Roboto-Regular.ttf",
	#ifdef GY_PLATFORM_WINDOWS
			"C:/Windows/Fonts/segoeui.ttf",
			"C:/Windows/Fonts/arial.ttf",
	#elif defined(GY_PLATFORM_MACOS)
			"/System/Library/Fonts/SFNS.ttf",
			"/Library/Fonts/Arial.ttf",
	#else
			"/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
			"/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
	#endif
		};

		// A monospaced face for anything columnar -- stats, log output, code.
		const char* s_MonoFontCandidates[] = {
			"assets/fonts/JetBrainsMono-Regular.ttf",
	#ifdef GY_PLATFORM_WINDOWS
			"C:/Windows/Fonts/CascadiaMono.ttf",
			"C:/Windows/Fonts/consola.ttf",
	#elif defined(GY_PLATFORM_MACOS)
			"/System/Library/Fonts/Menlo.ttc",
	#else
			"/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
	#endif
		};

		ImFont* TryLoadFirst(ImGuiIO& io, const char* const* candidates, size_t count, float sizePx)
		{
			for (size_t i = 0; i < count; i++)
			{
				if (!std::filesystem::exists(candidates[i]))
					continue;

				if (ImFont* font = io.Fonts->AddFontFromFileTTF(candidates[i], sizePx))
				{
					GY_CORE_INFO("Loaded font '{0}' at {1}px", candidates[i], sizePx);
					return font;
				}
			}
			return nullptr;
		}
	}

	void ImGuiLayer::LoadFonts()
	{
		ImGuiIO& io = ImGui::GetIO();

		// ImGui's built-in font is a 13px bitmap face that cannot scale and
		// looks dated on a modern display. A hinted TTF at 17px is the single
		// cheapest change to how the editor reads.
		const float uiSize = 17.0f;
		const float monoSize = 15.0f;

		ImFont* ui = TryLoadFirst(io, s_UIFontCandidates,
			sizeof(s_UIFontCandidates) / sizeof(s_UIFontCandidates[0]), uiSize);

		m_MonoFont = TryLoadFirst(io, s_MonoFontCandidates,
			sizeof(s_MonoFontCandidates) / sizeof(s_MonoFontCandidates[0]), monoSize);

		if (ui)
			io.FontDefault = ui;
		else
			GY_CORE_WARN("No UI font found; falling back to the ImGui built-in face");
	}

	void ImGuiLayer::SetStyle()
	{
		ImGuiStyle& style = ImGui::GetStyle();

		// Rounded, roomier, and with visible separation between panels. The
		// defaults are tuned for a debug overlay, not for an editor someone
		// looks at for hours.
		style.WindowRounding    = 6.0f;
		style.ChildRounding     = 6.0f;
		style.FrameRounding     = 4.0f;
		style.PopupRounding     = 6.0f;
		style.ScrollbarRounding = 8.0f;
		style.GrabRounding      = 4.0f;
		style.TabRounding       = 4.0f;

		style.WindowPadding     = ImVec2(10.0f, 10.0f);
		style.FramePadding      = ImVec2(8.0f, 5.0f);
		style.ItemSpacing       = ImVec2(8.0f, 7.0f);
		style.ItemInnerSpacing  = ImVec2(6.0f, 5.0f);
		style.IndentSpacing     = 20.0f;
		style.ScrollbarSize     = 12.0f;
		style.GrabMinSize       = 10.0f;

		style.WindowBorderSize  = 1.0f;
		style.FrameBorderSize   = 0.0f;
		style.PopupBorderSize   = 1.0f;

		style.WindowTitleAlign  = ImVec2(0.0f, 0.5f);
		style.WindowMenuButtonPosition = ImGuiDir_None;   // no collapse arrow

		// Curves and edges look wrong without enough segments at this scale.
		style.CircleTessellationMaxError = 0.15f;
		style.CurveTessellationTol = 0.8f;
	}

	void ImGuiLayer::SetDarkThemeColors()
	{
		auto& colors = ImGui::GetStyle().Colors;
		colors[ImGuiCol_WindowBg] = ImVec4{ 0.1f, 0.105f, 0.11f, 1.0f };

		// Headers
		colors[ImGuiCol_Header]        = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
		colors[ImGuiCol_HeaderHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
		colors[ImGuiCol_HeaderActive]  = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

		// Buttons
		colors[ImGuiCol_Button]        = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
		colors[ImGuiCol_ButtonHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
		colors[ImGuiCol_ButtonActive]  = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

		// Frame BG
		colors[ImGuiCol_FrameBg]        = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
		colors[ImGuiCol_FrameBgHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
		colors[ImGuiCol_FrameBgActive]  = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

		// Tabs
		colors[ImGuiCol_Tab]         = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
		colors[ImGuiCol_TabHovered]  = ImVec4{ 0.38f, 0.3805f, 0.381f, 1.0f };
		colors[ImGuiCol_TabSelected] = ImVec4{ 0.28f, 0.2805f, 0.281f, 1.0f };

		// Title
		colors[ImGuiCol_TitleBg]          = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
		colors[ImGuiCol_TitleBgActive]    = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
	}
}
