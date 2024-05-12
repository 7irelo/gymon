#pragma once

#ifdef GY_PLATFORM_WINDOWS

extern Gymon::Application* Gymon::CreateApplication();

int main(int argc, char **argv)
{
	Gymon::Log::Init();
	GY_CORE_WARN("Initialized Log!");
	int a = 5;
	GY_INFO("Hello! Var={0}", a);

	auto app = Gymon::CreateApplication();
	app->Run();
	delete app;
}

#endif
