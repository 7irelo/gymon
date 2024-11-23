#pragma once

#include "Core.h"
#include "Events/Event.h"
#include "Window.h"

namespace Gymon {

	class GYMON_API Application
	{
	public:
		Application();
		
		void Run();
		
		virtual ~Application();

	private:
		std::unique_ptr<Window> m_Window;
		bool m_Running = true;
	};

	// To be defined in Client
	Application* CreateApplication();

}


