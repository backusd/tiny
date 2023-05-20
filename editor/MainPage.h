#pragma once

#include "MainPage.g.h"
#include "../tiny/src/tiny.h"

namespace winrt::editor::implementation
{
    struct MainPage : MainPageT<MainPage>
    {
        MainPage()
        {
            // Xaml objects should not call InitializeComponent during construction.
            // See https://github.com/microsoft/cppwinrt/tree/master/nuget#initializecomponent
        
            m_timer.Reset();
            m_timer.Start();
        }

        int32_t MyProperty();
        void MyProperty(int32_t value);

        void ClickHandler(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& args);
    
        tiny::Timer m_timer;
    };
}

namespace winrt::editor::factory_implementation
{
    struct MainPage : MainPageT<MainPage, implementation::MainPage>
    {
    };
}
