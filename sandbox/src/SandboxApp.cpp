#include "SandboxApp.h"
#include "tiny/exception/TinyException.h"


static constexpr std::array g_textureFiles{
    L"C:/dev/tiny/sandbox/src/textures/grass.dds",		// 0
    L"C:/dev/tiny/sandbox/src/textures/water1.dds",		// 1
    L"C:/dev/tiny/sandbox/src/textures/WireFence.dds",	// 2
    L"C:/dev/tiny/sandbox/src/textures/bricks.dds"	    // 3
};

std::wstring GetTextureFilename(unsigned int index)
{
    return g_textureFiles[index];
}
std::size_t GetTotalTextureCount()
{
    return static_cast<std::size_t>(TEXTURE::Count);
}

namespace sandbox
{
Sandbox::Sandbox() :
    m_timer()
{
    PROFILE_BEGIN_SESSION("Startup", "profile/Profile-Startup.json");
    {
        PROFILE_SCOPE("Sandbox Constructor");

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

    PROFILE_END_SESSION();
}
bool Sandbox::DoFrame() noexcept
{
    PROFILE_NEXT_FRAME();

	try
	{
        PROFILE_SCOPE("Sandbox::DoFrame()");

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
    m_app->Update(timer);
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
    PROFILE_FUNCTION();

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
void Sandbox::OnKeyPressed(tiny::KeyPressedEvent& e) 
{
    switch (e.GetKeyCode())
    {
    case tiny::KEY_CODE::W: m_app->OnWKeyUpDown(true); break;
    case tiny::KEY_CODE::A: m_app->OnAKeyUpDown(true); break;
    case tiny::KEY_CODE::S: m_app->OnSKeyUpDown(true); break;
    case tiny::KEY_CODE::D: m_app->OnDKeyUpDown(true); break;
    }
}
void Sandbox::OnKeyReleased(tiny::KeyReleasedEvent& e) 
{
    switch (e.GetKeyCode())
    {
    case tiny::KEY_CODE::W: m_app->OnWKeyUpDown(false); break;
    case tiny::KEY_CODE::A: m_app->OnAKeyUpDown(false); break;
    case tiny::KEY_CODE::S: m_app->OnSKeyUpDown(false); break;
    case tiny::KEY_CODE::D: m_app->OnDKeyUpDown(false); break;

#ifdef PROFILE
    case tiny::KEY_CODE::P: tiny::Instrumentor::Get().CaptureFrames(5, "Frame Capture", "profile/Profile-Frames.json"); break;
#endif
    }
}

// REQUIRED FOR CRTP - Mouse Events
void Sandbox::OnMouseMove(tiny::MouseMoveEvent& e) 
{
    m_app->OnMouseMove(e.GetX(), e.GetY());
}
void Sandbox::OnMouseEnter(tiny::MouseEnterEvent& e) {}
void Sandbox::OnMouseLeave(tiny::MouseLeaveEvent& e) {}
void Sandbox::OnMouseScrolledVertical(tiny::MouseScrolledEvent& e) {}
void Sandbox::OnMouseScrolledHorizontal(tiny::MouseScrolledEvent& e) {}
void Sandbox::OnMouseButtonPressed(tiny::MouseButtonPressedEvent& e) 
{
    if (e.GetMouseButton() == tiny::MOUSE_BUTTON::LBUTTON)
    {
        m_app->OnLButtonUpDown(true);
    }
}
void Sandbox::OnMouseButtonReleased(tiny::MouseButtonReleasedEvent& e) 
{
    if (e.GetMouseButton() == tiny::MOUSE_BUTTON::LBUTTON)
    {
        m_app->OnLButtonUpDown(false);
    }
}
void Sandbox::OnMouseButtonDoubleClick(tiny::MouseButtonDoubleClickEvent& e) {}

}