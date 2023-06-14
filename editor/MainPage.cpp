#include "pch.h"
#include "MainPage.h"
#include "MainPage.g.cpp"
#include <concrt.h>



using namespace winrt::Windows::ApplicationModel;
using namespace winrt::Windows::Graphics::Display;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::System;
using namespace winrt::Windows::UI::Core;
using namespace winrt::Windows::UI::Input;
using namespace winrt::Windows::UI::Xaml::Controls;
using namespace winrt::Windows::UI::Xaml;
using namespace winrt::Windows::UI::Xaml::Interop;
using namespace winrt::Windows::UI::Xaml::Input;



namespace winrt::editor::implementation
{
    MainPage::MainPage() :
        m_windowVisible(false)
    {
        // Xaml objects should not call InitializeComponent during construction.
        // See https://github.com/microsoft/cppwinrt/tree/master/nuget#initializecomponent

        CoreWindow coreWindow = Window::Current().CoreWindow();
        coreWindow.VisibilityChanged({ this, &MainPage::OnVisibilityChanged });
        coreWindow.SizeChanged({ this, &MainPage::OnWindowSizeChanged });
        coreWindow.PointerMoved({ this, &MainPage::OnWindowPointerMoved });
        coreWindow.PointerPressed({ this, &MainPage::OnWindowPointerPressed });
        coreWindow.PointerReleased({ this, &MainPage::OnWindowPointerReleased });
        coreWindow.KeyUp({ this, &MainPage::OnWindowKeyUp });
        coreWindow.KeyDown({ this, &MainPage::OnWindowKeyDown });

        Window window = Window::Current();
        window.Activated({ this, &MainPage::OnWindowActivationChanged });

        DisplayInformation displayInformation = DisplayInformation::GetForCurrentView();
        displayInformation.DpiChanged({ this, &MainPage::OnDpiChanged });
        displayInformation.OrientationChanged({ this, &MainPage::OnOrientationChanged });
        displayInformation.StereoEnabledChanged({ this, &MainPage::OnStereoEnabledChanged });
        DisplayInformation::DisplayContentsInvalidated({ this, &MainPage::OnDisplayContentsInvalidated });

        // Disable all pointer visual feedback for better performance when touching.
        // TODO: Understand the next 3 lines
        auto pointerVisualizationSettings = PointerVisualizationSettings::GetForCurrentView();
        pointerVisualizationSettings.IsContactFeedbackEnabled(false);
        pointerVisualizationSettings.IsBarrelButtonFeedbackEnabled(false);

        // Create the device resources here, but NOTE that the swap chain will be created at a dummy size
        // It will be updated properly once the SwapChainPanel UI element gets created and triggers a SizeChanged event
        m_deviceResources = std::make_shared<tiny::DeviceResources>();

        m_scene = std::make_unique<Scene>(m_deviceResources, this);
    }
    void MainPage::InitializeComponent()
    {
        // From: https://github.com/microsoft/cppwinrt/tree/master/nuget#initializecomponent
        // "In older versions of C++/WinRT, Xaml objects called InitializeComponent from constructors. This can lead to memory corruption if 
        //  InitializeComponent throws an exception. C++/WinRT now calls InitializeComponent automatically and safely, after object construction. 
        //  Explicit calls to InitializeComponent from constructors in existing code should now be removed. Multiple calls to InitializeComponent 
        //  are idempotent. If a Xaml object needs to access a Xaml property during initialization, it should override InitializeComponent."
        // 
        // Call base InitializeComponent() to register with the Xaml runtime
        MainPageT::InitializeComponent();

        IDXGISwapChain1* swapChain = m_deviceResources->GetSwapChain();
        auto panelNative{ DXSwapChainPanel().as<ISwapChainPanelNative>() };

        winrt::check_hresult( 
            panelNative->SetSwapChain(swapChain)
        );


        // Start Render Loop
        m_scene->StartRenderLoop();
    }

    // Forwarded From App
    void MainPage::OnSuspending(const IInspectable&, const SuspendingEventArgs&)
    {
        concurrency::critical_section::scoped_lock lock(m_scene->GetCriticalSection());
        m_scene->Suspend();
        m_scene->StopRenderLoop();
    }
    void MainPage::OnResuming(const IInspectable&, const IInspectable&)
    {
        m_scene->Resume();
        m_scene->StartRenderLoop();
    }

    // CoreWindow
    void MainPage::OnVisibilityChanged(const CoreWindow&, const VisibilityChangedEventArgs& args)
    {
        m_windowVisible = args.Visible();
        if (m_windowVisible)
        {
            m_scene->StartRenderLoop();
        }
        else
        {
            m_scene->StopRenderLoop();
        }
    }
    void MainPage::OnWindowSizeChanged(const CoreWindow&, const WindowSizeChangedEventArgs&)
    {
        // This would be for updates for UI elements other than the SwapChainPanel because we handle DXSwapChainPanel_SizeChanged elsewhere
    }
    void MainPage::OnWindowPointerMoved(const CoreWindow& sender, const PointerEventArgs& args)
    {
        concurrency::critical_section::scoped_lock lock(m_scene->GetCriticalSection());
        Point p = args.CurrentPoint().Position();
        m_scene->OnMouseMove(p.X, p.Y);
    }
    void MainPage::OnWindowPointerPressed(const CoreWindow& sender, const PointerEventArgs& args)
    {
        concurrency::critical_section::scoped_lock lock(m_scene->GetCriticalSection());
        m_scene->OnLButtonUpDown(true);
    }
    void MainPage::OnWindowPointerReleased(const CoreWindow& sender, const PointerEventArgs& args)
    {
        concurrency::critical_section::scoped_lock lock(m_scene->GetCriticalSection());
        m_scene->OnLButtonUpDown(false);
    }
    void MainPage::OnWindowKeyUp(const CoreWindow& sender, const KeyEventArgs& args)
    {
        concurrency::critical_section::scoped_lock lock(m_scene->GetCriticalSection());
        switch (args.VirtualKey())
        {
        case VirtualKey::W: m_scene->OnWKeyUpDown(false); break;
        case VirtualKey::A: m_scene->OnAKeyUpDown(false); break;
        case VirtualKey::S: m_scene->OnSKeyUpDown(false); break;
        case VirtualKey::D: m_scene->OnDKeyUpDown(false); break;
        }
    }
    void MainPage::OnWindowKeyDown(const CoreWindow& sender, const KeyEventArgs& args)
    {
        concurrency::critical_section::scoped_lock lock(m_scene->GetCriticalSection());
        switch (args.VirtualKey())
        {
        case VirtualKey::W: m_scene->OnWKeyUpDown(true); break;
        case VirtualKey::A: m_scene->OnAKeyUpDown(true); break;
        case VirtualKey::S: m_scene->OnSKeyUpDown(true); break;
        case VirtualKey::D: m_scene->OnDKeyUpDown(true); break;
        }
    }

    // Window
    void MainPage::OnWindowActivationChanged(const IInspectable&, const WindowActivatedEventArgs& args)
    {
        concurrency::critical_section::scoped_lock lock(m_scene->GetCriticalSection());
        m_scene->WindowActivationChanged(args.WindowActivationState());
    }

    // DisplayInformation
    void MainPage::OnDpiChanged(const DisplayInformation&, const IInspectable&)
    {
        // Not handling this at the moment because DPI changes seem to only be important for text rendering (D2D)
    }
    void MainPage::OnOrientationChanged(const DisplayInformation&, const IInspectable&)
    {
        // Not handling because we are currently only support a standard orientation
    }
    void MainPage::OnStereoEnabledChanged(const DisplayInformation&, const IInspectable&)
    {
        // Not handling because we are not going into into a stereo enabled mode
    }
    void MainPage::OnDisplayContentsInvalidated(const DisplayInformation&, const IInspectable&)
    {
        concurrency::critical_section::scoped_lock lock(m_scene->GetCriticalSection());
       
        // TODO:
        //m_deviceResources->ValidateDevice();
    }

    void MainPage::DXSwapChainPanel_SizeChanged(IInspectable const&, SizeChangedEventArgs const& e)
    {
        concurrency::critical_section::scoped_lock lock(m_scene->GetCriticalSection());

        int height = static_cast<int>(e.NewSize().Height);
        int width = static_cast<int>(e.NewSize().Width);

        m_deviceResources->OnResize(height, width);
        m_scene->OnResize(height, width);
    }
    void MainPage::DXSwapChainPanel_CompositionScaleChanged(SwapChainPanel const& /*sender*/, IInspectable const&)
    {
        // Not worrying about this because I don't know what it does lol
    }
    void MainPage::ViewportGrid_SizeChanged(IInspectable const& sender, SizeChangedEventArgs const& e)
    {
        concurrency::critical_section::scoped_lock lock(m_scene->GetCriticalSection());
        
        auto viewportGrid = sender.as<Controls::Grid>();
        
        float top = viewportGrid.ActualOffset().y;
        float left = viewportGrid.ActualOffset().x;
        
        float height = e.NewSize().Height;
        float width = e.NewSize().Width;
        
        m_scene->SetViewport(top, left, height, width);
    }

}






