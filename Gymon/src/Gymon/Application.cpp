#include "gypch.h"
#include "Application.h"

#include "Gymon/Events/ApplicationEvent.h"
#include "Gymon/Log.h"
#include "Events/MouseEvent.h"

namespace Gymon {

	Application::Application()
	{
		m_Window = std::unique_ptr<Window>(Window::Create());
	}

	Application::~Application()
	{

	}

	void Application::Run()
	{
		while (m_Running)
		{
			m_Window->OnUpdate();
		}
	}
}