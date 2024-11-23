#include "Application.h"

#include "Gymon/Events/ApplicationEvent.h"
#include "Gymon/Log.h"
#include "Events/MouseEvent.h"

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
		MouseMovedEvent f(20, 30);
		if (e.IsInCategory(EventCategoryApplication))
		{
			GY_TRACE(e.ToString());
		}
		if (e.IsInCategory(EventCategoryInput))
		{
			GY_TRACE(e.ToString());
		}
		if (f.IsInCategory(EventCategoryMouse))
		{
			GY_ERROR(f.ToString());
		}
		
		while (true);
	}
}