#pragma once

#include "Gymon/Core.h"

#ifdef GY_PLATFORM_WINDOWS

extern Gymon::Application* Gymon::CreateApplication();

int main(int argc, char** argv)
{
	Gymon::Log::Init();
	GY_CORE_INFO("Gymon Engine starting up...");

	auto app = Gymon::CreateApplication();
	app->Run();
	delete app;

	return 0;
}

#endif
