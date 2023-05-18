#pragma once
#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

extern tiny::IApplication* tiny::CreateApplication();

int main(int argc, char** argv)
{
    // NOTE: I'm going to disable memory leak checks for now because I currently use std::chrono::current_zone()
    // in the function tiny::log::current_time_and_date(), and this is causing memory leak checks to signal that
    // a memory leak has taken place. However, according to this post (https://developercommunity.visualstudio.com/t/reported-memory-leak-when-converting-file-time-typ/1467739)
    //    "Because the Standard depicts tzdb as using vector<leap_second>, vector<time_zone>, and vector<time_zone_link> 
    //     in [time.zone.tzdb], we are required to use std::allocator in our implementation, rather than use CRT 
    //     allocations which would not show up as memory leaks. If you are using tzdb (or pieces of chrono that use tzdb 
    //     indirectly such as clock_cast) and want to use CRT leak checking, you should load the tzdb and then take a CRT 
    //     heap snapshot."
    // This is more effort than I think is necessary right now as I make extensive use of smart pointers and DirectX
    // also does outstanding reference count checking on DirectX objects.
    // 
    // Enable run-time memory check for debug builds.
//#if defined(DEBUG) | defined(_DEBUG)
//    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
//#endif

    try
    {
        std::unique_ptr<tiny::IApplication> app = std::unique_ptr<tiny::IApplication>(tiny::CreateApplication());
        app->Run();
    }
    catch (const tiny::TinyAppException& e)
    {
        MessageBox(nullptr, e.what(), e.GetType(), MB_OK | MB_ICONEXCLAMATION);
    }
    catch (const std::exception& e)
    {
        MessageBox(nullptr, e.what(), "Standard Exception", MB_OK | MB_ICONEXCLAMATION);
    }
    catch (...)
    {
        MessageBox(nullptr, "No details available", "Unknown Exception", MB_OK | MB_ICONEXCLAMATION);
    }

    return 0;
}