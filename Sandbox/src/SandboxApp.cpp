#include <Gymon.h>

class Sandbox : public Gymon::Application
{
public:
	Sandbox()
	{

	}
	~Sandbox()
	{

	}
};

Gymon::Application* Gymon::CreateApplication()
{
	return new Sandbox();
}