#pragma once

#include "Core.h"
#include "Window.h"
#include "Gymon/LayerStack.h"
#include "Gymon/Events/Event.h"
#include "Gymon/Events/ApplicationEvent.h"
#include "Gymon/Core/Timestep.h"

#include "Gymon/ImGui/ImGuiLayer.h"

namespace Gymon {

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
	private:
		static Application* s_Instance;
	};

	// To be defined in client
	Application* CreateApplication();
}
