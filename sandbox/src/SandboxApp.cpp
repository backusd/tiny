#include <tiny-app.h>
#include <tiny-app/EntryPoint.h>
#include <tiny.h>

class Sandbox : public tiny::Application
{
public:
	Sandbox()
	{
	}
	Sandbox(const Sandbox&) = delete;
	Sandbox& operator=(const Sandbox&) = delete;
	~Sandbox() override {}
};

tiny::Application* tiny::CreateApplication()
{
	return new Sandbox();
}