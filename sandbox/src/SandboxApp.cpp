#include "SandboxApp.h"
#include "tiny/exception/TinyException.h"

namespace sandbox
{
Sandbox::Sandbox()
{
}
bool Sandbox::DoFrame() noexcept
{
	try
	{
		Update();
		Render();
		Present();        
	}
    catch (const tiny::TinyException& e)
    {
        LOG_ERROR("{}", "Caught TinyException:");
        LOG_ERROR("Type: {}", e.GetType());
        LOG_ERROR("Details: {}", e.what());
        return false;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("{}", "Caught std::exception:");
        LOG_ERROR("Type: {}", "Standard Exception");
        LOG_ERROR("Details: {}", e.what());
        return false;
    }
    catch (...)
    {
        LOG_ERROR("{}", "Caught exception:");
        LOG_ERROR("Type: {}", "Unknown");
        LOG_ERROR("Details: {}", "No details available");
        return false;
    }

    return true;
}

void Sandbox::Update() 
{

}
void Sandbox::Render() 
{

}
void Sandbox::Present() 
{

}




}