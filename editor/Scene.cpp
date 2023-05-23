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
    m_app = std::make_unique<tiny::TheApp>(m_deviceResources);
    m_app->SetViewport(
        0.0f,
        0.0f,
        static_cast<float>(m_deviceResources->GetHeight()),
        static_cast<float>(m_deviceResources->GetWidth())
    );
}

void Scene::OnResize(int height, int width)
{
    // Make any necessary updates here, then start the render loop if necessary
    // ...
    m_app->OnResize(height, width);
    m_app->SetViewport(
        0.0f,
        0.0f,
        static_cast<float>(height),
        static_cast<float>(width)
    );



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
                m_app->Update(timer);
                

                // Render =========================================================================
                //
                m_app->Render();

                // Present ========================================================================
                m_app->Present();

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
