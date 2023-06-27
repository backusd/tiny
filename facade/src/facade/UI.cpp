#include "facade-pch.h"
#include "UI.h"

//#include <boost/asio/signal_set.hpp>
//#include <boost/smart_ptr.hpp>

namespace facade
{
UI::UI()
{
    // Kick off the UI thread and detach it so we don't have to wait on it
    // It will run persistantly in the background
    //std::thread uiThread(&UI::StartUIThread, this); <-- NOTE: do NOT use this, it does not work for some reason. Seems to not actually create a new thread.

//    std::thread uiThread([this]() { StartUIThread(); });
//    uiThread.detach();
}

void UI::StartUIThread()
{
//    std::cout << "UI running on thread: " << std::this_thread::get_id() << std::endl;
//
//    // Whenever we make changes, we need to control the critical section
//    concurrency::critical_section::scoped_lock lock(m_criticalSection);
//
//    auto address = net::ip::make_address("0.0.0.0");
//    unsigned short port = 8080;
//
//    // Create and launch a listening port
//    boost::make_shared<_Listener>(this, m_ioc, tcp::endpoint{ address, port })->Run();
//
//    // Create threads for the io_context
//    Run();
}

void UI::Run()
{
//    // Capture SIGINT and SIGTERM to perform a clean shutdown
//    net::signal_set signals(m_ioc, SIGINT, SIGTERM);
//    signals.async_wait(
//        [this](boost::system::error_code const&, int)
//        {
//            // Stop the io_context. This will cause run()
//            // to return immediately, eventually destroying the
//            // io_context and any remaining handlers in it.
//            m_ioc.stop();
//        });
//
//    // Run the I/O service on the requested number of threads
//    auto const threads = 2;
//    std::vector<std::thread> v;
//    v.reserve(threads - 1);
//    for (auto i = threads - 1; i > 0; --i)
//        v.emplace_back(
//            [this]
//            {
//                m_ioc.run();
//            });
//    m_ioc.run();
//
//    // (If we get here, it means we got a SIGINT or SIGTERM)
//
//    // Block until all the threads exit
//    for (auto& t : v)
//        t.join();
}


// Specialize the string template because this is the template that will do the actual sending of data
template<>
void UI::SendMsgImpl<std::string>(const std::string& data)
{
//    if (m_webSocketSession != nullptr)
//    {
//        m_webSocketSession->Send(data);
//    }
}

void UI::HandleMessageImpl(const std::string& message)
{
//    try
//    {
//        json data = json::parse(message);
//        std::cout << "json: " << data.dump(4) << std::endl;
//
//        if (!data.contains("type"))
//            throw std::exception::exception("JSON data does not contain 'type' key");
//
//        std::string type = data["type"].get<std::string>();
//
//        if (m_handlers.find(type) == m_handlers.end())
//            std::cout << std::format("WARNING: No handler with type = {}", type) << std::endl;
//        else
//        {
//            // Get control of the critical section before executing the handler
//            concurrency::critical_section::scoped_lock lock(m_criticalSection);
//            m_handlers[type](data);
//        }
//    }
//    catch (json::parse_error& e)
//    {
//        std::cout << "Caught json::parse_error: " << e.what() << std::endl;
//    }
}
void UI::HandleMessageImpl(std::string&& message)
{
    std::string s = std::move(message);
    HandleMessageImpl(s);
}


void UI::SetMsgHandlerImpl(const std::string& key, std::function<void(const json&)> func)
{
    // Get control of the critical section before adding a handler
    concurrency::critical_section::scoped_lock lock(m_criticalSection);
    m_handlers[key] = func;
}

}