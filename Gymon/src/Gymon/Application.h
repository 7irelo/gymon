#pragma once

#include "Core.h"
#include "Window.h"
#include "Gymon/LayerStack.h"
#include "Gymon/Events/Event.h"
#include "Gymon/Events/ApplicationEvent.h"
#include "Gymon/Core/Timestep.h"

#include "Gymon/ImGui/ImGuiLayer.h"

namespace Gymon {

	// Options parsed from argv by EntryPoint before the application is built.
	//
	// The screenshot options exist so the renderer can be checked from a
	// script: run the app, let it settle for a few frames, write a PNG, exit.
	// Grabbing the window off the desktop instead would capture whatever else
	// happens to be on screen and depends on the window having focus.
	struct CommandLineOptions
	{
		std::string ScreenshotPath;
		// Frames to render before capturing. The first frames show default
		// ImGui layout and an unsettled camera, so capturing frame 0 is not
		// representative.
		uint32_t ScreenshotFrame = 60;
		// Set by --screenshot: an automated capture wants the process to end.
		bool ExitAfterScreenshot = false;
	};

	class Application
	{
	public:
		Application(const std::string& name = "Gymon Engine");
		virtual ~Application();

		void Run();

		void OnEvent(Event& e);

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* overlay);

		Window& GetWindow() { return *m_Window; }

		void Close();

		ImGuiLayer* GetImGuiLayer() { return m_ImGuiLayer; }

		static Application& Get() { return *s_Instance; }

		// Mutable so EntryPoint can fill it in before CreateApplication runs.
		static CommandLineOptions& Options();
	private:
		bool OnWindowClose(WindowCloseEvent& e);
		bool OnWindowResize(WindowResizeEvent& e);
	private:
		Scope<Window> m_Window;
		ImGuiLayer* m_ImGuiLayer;
		bool m_Running = true;
		bool m_Minimized = false;
		LayerStack m_LayerStack;
		float m_LastFrameTime = 0.0f;
		uint32_t m_FrameCount = 0;
	private:
		static Application* s_Instance;
	};

	// To be defined in client
	Application* CreateApplication();
}
