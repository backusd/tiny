#include <tiny-app.h>
#include <tiny-app/EntryPoint.h>
#include "src/SandboxApp.h"

tiny::IApplication* tiny::CreateApplication()
{
	return new sandbox::Sandbox();
}