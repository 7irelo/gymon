#include "Application.h"

#include "Gymon/Events/ApplicationEvent.h"
#include "Gymon/Log.h"

namespace Gymon {

	Application::Application()
	{

	}

	Application::~Application()
	{

	}

	void Application::Run()
	{
		WindowResizeEvent e(1200, 720);
		GY_TRACE(e.ToString());
		while (true);
	}
}