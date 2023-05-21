#pragma once
#include "pch.h"
#include "MainPage.g.h"
#include "../tiny/src/tiny.h"
#include "ISceneUIControl.h"
#include "Scene.h"


namespace winrt::editor::implementation
{
    struct MainPage : MainPageT<MainPage>, ISceneUIControl
    {
        MainPage();
        void InitializeComponent();

        // Forwarded from App
        void OnSuspending(const Windows::Foundation::IInspectable&, const Windows::ApplicationModel::SuspendingEventArgs&);
        void OnResuming(const Windows::Foundation::IInspectable&, const Windows::Foundation::IInspectable&);

        // Core Window
        void OnVisibilityChanged(const Windows::UI::Core::CoreWindow& sender, const Windows::UI::Core::VisibilityChangedEventArgs& args);
        void OnWindowSizeChanged(const Windows::UI::Core::CoreWindow& sender, const Windows::UI::Core::WindowSizeChangedEventArgs& args);

        // Window
        void OnWindowActivationChanged(const Windows::Foundation::IInspectable& sender, const Windows::UI::Core::WindowActivatedEventArgs& args);

        // Display Information
        void OnDpiChanged(const Windows::Graphics::Display::DisplayInformation& displayInfo, const Windows::Foundation::IInspectable& args);
        void OnOrientationChanged(const Windows::Graphics::Display::DisplayInformation& displayInfo, const Windows::Foundation::IInspectable& args);
        void OnStereoEnabledChanged(const Windows::Graphics::Display::DisplayInformation& displayInfo, const Windows::Foundation::IInspectable& args);
        void OnDisplayContentsInvalidated(const Windows::Graphics::Display::DisplayInformation& displayInfo, const Windows::Foundation::IInspectable& args);

        // ISceneUIControl
        virtual void SetSceneLoading() override
        {
        }

        // SwapChainPanel
        void DXSwapChainPanel_SizeChanged(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::UI::Xaml::SizeChangedEventArgs const& e);
        void DXSwapChainPanel_CompositionScaleChanged(winrt::Windows::UI::Xaml::Controls::SwapChainPanel const& sender, winrt::Windows::Foundation::IInspectable const& args);
        
        // Viewport Grid
        void ViewportGrid_SizeChanged(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::UI::Xaml::SizeChangedEventArgs const& e);

    private:
        bool m_windowVisible;

        std::shared_ptr<tiny::DeviceResources> m_deviceResources;
        std::unique_ptr<Scene> m_scene;
    };
}

namespace winrt::editor::factory_implementation
{
    struct MainPage : MainPageT<MainPage, implementation::MainPage>
    {
    };
}
