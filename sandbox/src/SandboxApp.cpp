#include "SandboxApp.h"
#include "tiny/exception/TinyException.h"
#include "tiny/utils/Utility.h"

namespace sandbox
{
Sandbox::Sandbox() :
    m_timer()
{
    m_deviceResources = std::make_shared<tiny::DeviceResources>(GetHWND(), GetWindowHeight(), GetWindowWidth());
    TINY_ASSERT(m_deviceResources != nullptr, "Failed to create device resources");

    m_app = std::make_unique<tiny::TheApp>(m_deviceResources);
    m_app->SetViewport(
        0.0f, 
        0.0f, 
        static_cast<float>(GetWindowHeight()), 
        static_cast<float>(GetWindowWidth())
    );

    m_timer.Reset();
}
bool Sandbox::DoFrame() noexcept
{
	try
	{
        m_timer.Tick();
        CalculateFrameStats();

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
    m_app->Update();
}
void Sandbox::Render() 
{
    m_app->Render();
}
void Sandbox::Present() 
{
    m_app->Present();
}

void Sandbox::CalculateFrameStats()
{
    // Code computes the average frames per second, and also the 
    // average time it takes to render one frame.  These stats 
    // are appended to the window caption bar.

    static int frameCnt = 0;
    static float timeElapsed = 0.0f;

    frameCnt++;

    // Compute averages over one second period.
    if ((m_timer.TotalTime() - timeElapsed) >= 1.0f)
    {
        float fps = (float)frameCnt; // fps = frameCnt / 1
        float mspf = 1000.0f / fps;

        std::wstring fpsStr = std::to_wstring(fps);
        std::wstring mspfStr = std::to_wstring(mspf);

        std::wstring windowText =
            L"    fps: " + fpsStr +
            L"   mspf: " + mspfStr;

        SetWindowText(GetHWND(), windowText.c_str());

        // Reset for next average.
        frameCnt = 0;
        timeElapsed += 1.0f;
    }
}

// REQUIRED FOR CRTP - Application Events
void Sandbox::OnWindowResize(tiny::WindowResizeEvent& e) 
{
    m_deviceResources->OnResize(e.GetHeight(), e.GetWidth());
    m_app->OnResize(e.GetHeight(), e.GetWidth());
    m_app->SetViewport(
        0.0f,
        0.0f,
        static_cast<float>(e.GetHeight()),
        static_cast<float>(e.GetWidth())
    );
}
void Sandbox::OnWindowCreate(tiny::WindowCreateEvent& e) {}
void Sandbox::OnWindowClose(tiny::WindowCloseEvent& e) {}
void Sandbox::OnAppTick(tiny::AppTickEvent& e) {}
void Sandbox::OnAppUpdate(tiny::AppUpdateEvent& e) {}
void Sandbox::OnAppRender(tiny::AppRenderEvent& e) {}

// REQUIRED FOR CRTP - Key Events
void Sandbox::OnChar(tiny::CharEvent& e) {}
void Sandbox::OnKeyPressed(tiny::KeyPressedEvent& e) {}
void Sandbox::OnKeyReleased(tiny::KeyReleasedEvent& e) {}

// REQUIRED FOR CRTP - Mouse Events
void Sandbox::OnMouseMove(tiny::MouseMoveEvent& e) {}
void Sandbox::OnMouseEnter(tiny::MouseEnterEvent& e) {}
void Sandbox::OnMouseLeave(tiny::MouseLeaveEvent& e) {}
void Sandbox::OnMouseScrolledVertical(tiny::MouseScrolledEvent& e) {}
void Sandbox::OnMouseScrolledHorizontal(tiny::MouseScrolledEvent& e) {}
void Sandbox::OnMouseButtonPressed(tiny::MouseButtonPressedEvent& e) {}
void Sandbox::OnMouseButtonReleased(tiny::MouseButtonReleasedEvent& e) {}
void Sandbox::OnMouseButtonDoubleClick(tiny::MouseButtonDoubleClickEvent& e) {}

}