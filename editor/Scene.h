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

    void CreateWindowSizeDependentResources();
    void StartRenderLoop();
    void StopRenderLoop();
    void Suspend();
    void Resume();

    concurrency::critical_section& GetCriticalSection() { return m_criticalSection; }

    void WindowActivationChanged(winrt::Windows::UI::Core::CoreWindowActivationState activationState);


    // Rendering Stuff
    inline void SetViewport(float top, float left, float height, float width) { m_deviceResources->SetViewport(top, left, height, width); }


private:
    bool m_haveFocus;

    std::shared_ptr<tiny::DeviceResources> m_deviceResources;
    ISceneUIControl* m_uiControl;

    concurrency::critical_section            m_criticalSection;
    winrt::Windows::Foundation::IAsyncAction m_renderLoopWorker;

};