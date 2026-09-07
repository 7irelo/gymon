#pragma once

#include "Gymon/Core.h"
#include "Gymon/Application.h"

#include <cstdlib>
#include <string>

#ifdef GY_PLATFORM_WINDOWS

extern Gymon::Application* Gymon::CreateApplication();

int main(int argc, char** argv)
{
	Gymon::Log::Init();
	GY_CORE_INFO("Gymon Engine starting up...");

	// Parsed before the application exists so a layer can read the options in
	// its constructor if it ever needs to.
	auto& options = Gymon::Application::Options();
	for (int i = 1; i < argc; i++)
	{
		const std::string arg = argv[i];

		if (arg == "--screenshot" && i + 1 < argc)
		{
			options.ScreenshotPath = argv[++i];
			options.ExitAfterScreenshot = true;
		}
		else if (arg == "--screenshot-frame" && i + 1 < argc)
		{
			options.ScreenshotFrame = (uint32_t)std::strtoul(argv[++i], nullptr, 10);
		}
		else if (arg == "--help" || arg == "-h")
		{
			GY_CORE_INFO("Usage: Sandbox [--screenshot <path.png>] [--screenshot-frame <n>]");
			return 0;
		}
		else
		{
			GY_CORE_WARN("Ignoring unrecognised argument '{0}'", arg);
		}
	}

	auto app = Gymon::CreateApplication();
	app->Run();
	delete app;

	return 0;
}

#endif
