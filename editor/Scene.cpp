#include "pch.h"
#include "Scene.h"


using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::System::Threading;
using namespace winrt::Windows::UI::Core;


Scene::Scene(std::shared_ptr<tiny::DeviceResources> deviceResources, ISceneUIControl* uiControl) :
    m_deviceResources(deviceResources),
    m_uiControl(uiControl),
    m_haveFocus(false)
{
}

void Scene::CreateWindowSizeDependentResources()
{
    // Make any necessary updates here, then start the render loop if necessary
    // ...



    if (m_renderLoopWorker == nullptr || m_renderLoopWorker.Status() != AsyncStatus::Started)
    {
        StartRenderLoop();
    }
}
void Scene::StartRenderLoop()
{
    if (m_renderLoopWorker != nullptr && m_renderLoopWorker.Status() == AsyncStatus::Started)
    {
        return;
    }

    // Create a task that will be run on a background thread.
    auto workItemHandler = WorkItemHandler([this](IAsyncAction action)
        {
            tiny::Timer timer;
            timer.Reset();
            timer.Start();

            // Calculate the updated frame and render once per vertical blanking interval.
            while (action.Status() == AsyncStatus::Started)
            {
                concurrency::critical_section::scoped_lock lock(m_criticalSection);

                timer.Tick();

                // Update =========================================================================
                m_deviceResources->Update();
                

                // Render =========================================================================
                //
                m_deviceResources->Render();

                // Present ========================================================================
                m_deviceResources->Present();

                if (!m_haveFocus)
                {
                    // The app is in an inactive state so stop rendering
                    // This optimizes for power and allows the framework to become more queiecent
                    break;
                }
            }
        });

    // Run task on a dedicated high priority background thread.
    m_renderLoopWorker = ThreadPool::RunAsync(workItemHandler, WorkItemPriority::High, WorkItemOptions::TimeSliced);
}
void Scene::StopRenderLoop()
{
    m_renderLoopWorker.Cancel();
}
void Scene::Suspend()
{

}
void Scene::Resume()
{

}

void Scene::WindowActivationChanged(CoreWindowActivationState activationState)
{
    if (activationState == CoreWindowActivationState::Deactivated)
    {
        m_haveFocus = false;
    }
    else if (activationState == CoreWindowActivationState::CodeActivated ||
             activationState == CoreWindowActivationState::PointerActivated)
    {
        m_haveFocus = true;

        if (m_renderLoopWorker == nullptr || m_renderLoopWorker.Status() != AsyncStatus::Started)
        {
            StartRenderLoop();
        }
    }
}
