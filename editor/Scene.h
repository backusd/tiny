#pragma once
#include "pch.h"
#include "../tiny/src/tiny.h"
#include "ISceneUIControl.h"

class Scene
{
public:
    Scene(std::shared_ptr<tiny::DeviceResources> deviceResources, ISceneUIControl* uiControl);
    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;
    ~Scene() {}

    void OnResize(int height, int width);
    void StartRenderLoop();
    void StopRenderLoop();
    void Suspend();
    void Resume();

    concurrency::critical_section& GetCriticalSection() { return m_criticalSection; }

    void WindowActivationChanged(winrt::Windows::UI::Core::CoreWindowActivationState activationState);


    // Rendering Stuff
    inline void SetViewport(float top, float left, float height, float width) { m_app->SetViewport(top, left, height, width); }

    // Mouse/Keyboard Events
    void OnMouseMove(float x, float y) { m_app->OnMouseMove(x, y); }
    void OnLButtonUpDown(bool isDown) { m_app->OnLButtonUpDown(isDown); }
    void OnWKeyUpDown(bool isDown) noexcept { m_app->OnWKeyUpDown(isDown); }
    void OnAKeyUpDown(bool isDown) noexcept { m_app->OnAKeyUpDown(isDown); }
    void OnSKeyUpDown(bool isDown) noexcept { m_app->OnSKeyUpDown(isDown); }
    void OnDKeyUpDown(bool isDown) noexcept { m_app->OnDKeyUpDown(isDown); }

private:
    bool m_haveFocus;

    std::shared_ptr<tiny::DeviceResources> m_deviceResources;

    ISceneUIControl* m_uiControl;

    concurrency::critical_section            m_criticalSection;
    winrt::Windows::Foundation::IAsyncAction m_renderLoopWorker;

    // TEMPORARY: We use TheApp class to implement app-specific stuff in the library itself.
    //            Eventually this should be removed, but it serves as an easy way to run the code on Win32 and UWP right now
    std::unique_ptr<tiny::TheApp> m_app;
};