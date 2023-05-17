#include "SandboxApp.h"
#include "tiny/exception/TinyException.h"

namespace sandbox
{
Sandbox::Sandbox() :
    m_timer()
{
    m_timer.Reset();
}
bool Sandbox::DoFrame() noexcept
{
	try
	{
        m_timer.Tick();

		Update(m_timer);
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

void Sandbox::Update(const tiny::Timer& timer) 
{

}
void Sandbox::Render() 
{

}
void Sandbox::Present() 
{

}




}