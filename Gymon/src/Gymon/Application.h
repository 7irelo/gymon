#pragma once

#include "Core.h"
#include "Events/Event.h"

namespace Gymon {

	class GYMON_API Application
	{
	public:
		Application();
		
		void Run();
		
		virtual ~Application();
		
	};

	// To be defined in Client
	Application* CreateApplication();

}


