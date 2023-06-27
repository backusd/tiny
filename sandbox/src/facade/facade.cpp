#include "facade.h"

#include <boost/asio/signal_set.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/optional.hpp>

namespace facade
{
// ====================================================================================================
// Utility functions
// ====================================================================================================
 // Return a reasonable mime type based on the extension of a file.
beast::string_view mime_type(beast::string_view path)
{
    using beast::iequals;
    auto const ext = [&path]
    {
        auto const pos = path.rfind(".");
        if (pos == beast::string_view::npos)
            return beast::string_view{};
        return path.substr(pos);
    }();
    if (iequals(ext, ".htm"))  return "text/html";
    if (iequals(ext, ".html")) return "text/html";
    if (iequals(ext, ".php"))  return "text/html";
    if (iequals(ext, ".css"))  return "text/css";
    if (iequals(ext, ".txt"))  return "text/plain";
    if (iequals(ext, ".js"))   return "application/javascript";
    if (iequals(ext, ".json")) return "application/json";
    if (iequals(ext, ".xml"))  return "application/xml";
    if (iequals(ext, ".swf"))  return "application/x-shockwave-flash";
    if (iequals(ext, ".flv"))  return "video/x-flv";
    if (iequals(ext, ".png"))  return "image/png";
    if (iequals(ext, ".jpe"))  return "image/jpeg";
    if (iequals(ext, ".jpeg")) return "image/jpeg";
    if (iequals(ext, ".jpg"))  return "image/jpeg";
    if (iequals(ext, ".gif"))  return "image/gif";
    if (iequals(ext, ".bmp"))  return "image/bmp";
    if (iequals(ext, ".ico"))  return "image/vnd.microsoft.icon";
    if (iequals(ext, ".tiff")) return "image/tiff";
    if (iequals(ext, ".tif"))  return "image/tiff";
    if (iequals(ext, ".svg"))  return "image/svg+xml";
    if (iequals(ext, ".svgz")) return "image/svg+xml";
    return "application/text";
}

// Append an HTTP rel-path to a local filesystem path.
// The returned path is normalized for the platform.
std::string path_cat(beast::string_view base, beast::string_view path)
{
    if (base.empty())
        return std::string(path);
    std::string result(base);
#ifdef BOOST_MSVC
    char constexpr path_separator = '\\';
    if (result.back() == path_separator)
        result.resize(result.size() - 1);
    result.append(path.data(), path.size());
    for (auto& c : result)
        if (c == '/')
            c = path_separator;
#else
    char constexpr path_separator = '/';
    if (result.back() == path_separator)
        result.resize(result.size() - 1);
    result.append(path.data(), path.size());
#endif
    return result;
}

// ====================================================================================================
// UI
// ====================================================================================================
UI::UI()
{
    // Kick off the UI thread and detach it so we don't have to wait on it
    // It will run persistantly in the background
    //std::thread uiThread(&UI::StartUIThread, this); <-- NOTE: do NOT use this, it does not work for some reason. Seems to not actually create a new thread.

    std::thread uiThread([this]() { StartUIThread(); });
    uiThread.detach();
}

void UI::StartUIThread()
{
    // Create a scope for the scoped lock
    {
        // Hold the critical section until we call ioc.run()
        concurrency::critical_section::scoped_lock lock(m_criticalSection);

        const char* ip = "0.0.0.0";
        auto address = net::ip::make_address(ip);
        unsigned short port = 8080;

        // Create and launch a listening port
        boost::make_shared<Listener>(this, m_ioc, tcp::endpoint{ address, port })->Run();       
    }

    // NOTE: do NOT call signals.async_await() in an enclosing scope such that it goes out of scope before
    //       calling ioc.run(). If you do this, I'm not sure if SIGINT/SIGTERM is sent, but either way, the lambda
    //       is called which calls ioc.stop(). This results in the ioc threads immediately finishing
    // 
    // Capture SIGINT and SIGTERM to perform a clean shutdown
    net::signal_set signals(m_ioc, SIGINT, SIGTERM);
    signals.async_wait(
        [this](boost::system::error_code const&, int)
        {
            // Stop the io_context. This will cause run()
            // to return immediately, eventually destroying the
            // io_context and any remaining handlers in it.
            m_ioc.stop();
        }
    );

    // Run the I/O service on the requested number of threads
    auto const threads = 2;
    LOG_INFO("FACADE - Calling ioc.run() on {} threads", threads); 

    std::vector<std::thread> v;
    v.reserve(threads - 1);
    for (auto i = threads - 1; i > 0; --i)
        v.emplace_back(
            [this]
            {
                m_ioc.run();
            });
    m_ioc.run();

    // (If we get here, it means we got a SIGINT or SIGTERM)
    //
    // Block until all the threads exit
    for (auto& t : v)
        t.join();

    LOG_INFO("{}", "FACADE - All ioc threads have finished executing");
}

// Specialize the string template because this is the template that will do the actual sending of data
template<>
void UI::SendMsgImpl<std::string>(const std::string& data)
{
    if (m_webSocketSession != nullptr)
    {
        m_webSocketSession->Send(data);
    }
}

void UI::HandleMessageImpl(const std::string& message)
{
    try
    {
        json data = json::parse(message);
        std::cout << "json: " << data.dump(4) << std::endl;

        if (!data.contains("type"))
            throw std::exception::exception("JSON data does not contain 'type' key");

        std::string type = data["type"].get<std::string>();

        if (m_handlers.find(type) == m_handlers.end())
            std::cout << std::format("WARNING: No handler with type = {}", type) << std::endl;
        else
        {
            // Get control of the critical section before executing the handler
            concurrency::critical_section::scoped_lock lock(m_criticalSection);
            m_handlers[type](data);
        }
    }
    catch (json::parse_error& e)
    {
        std::cout << "Caught json::parse_error: " << e.what() << std::endl;
    }
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


// ====================================================================================================
// Listener
// ====================================================================================================
Listener::Listener(UI* ui, net::io_context& ioc, tcp::endpoint endpoint) :
    m_ui(ui),
    m_ioc(ioc),
    m_acceptor(ioc)
{
    beast::error_code ec;

    // Open the acceptor
    m_acceptor.open(endpoint.protocol(), ec);
    if (ec)
    {
        Fail(ec, "open");
        return;
    }

    // Allow address reuse
    m_acceptor.set_option(net::socket_base::reuse_address(true), ec);
    if (ec)
    {
        Fail(ec, "set_option");
        return;
    }

    // Bind to the server address
    m_acceptor.bind(endpoint, ec);
    if (ec)
    {
        Fail(ec, "bind");
        return;
    }

    // Start listening for connections
    m_acceptor.listen(
        net::socket_base::max_listen_connections, ec);
    if (ec)
    {
        Fail(ec, "listen");
        return;
    }

    LOG_INFO("FACADE - Listening on {}:{}", endpoint.address().to_string(), endpoint.port());
}

void Listener::Run()
{
    // The new connection gets its own strand
    m_acceptor.async_accept(
        net::make_strand(m_ioc),
        beast::bind_front_handler(
            &Listener::OnAccept,
            shared_from_this()
        )
    );
}

// Report a failure
void Listener::Fail(beast::error_code ec, char const* what)
{
    // Don't report on canceled operations
    if (ec == net::error::operation_aborted)
        return;
    LOG_ERROR("{}: {}", what, ec.message());
}

// Handle a connection
void Listener::OnAccept(beast::error_code ec, tcp::socket socket)
{
    if (ec)
        return Fail(ec, "accept");
    else
    {
        // Launch a new session for this connection
        boost::shared_ptr<HTTPSession> session = boost::make_shared<HTTPSession>(m_ui, std::move(socket));
        UI::SetHTTPSession(session);
        session->Run();
    }

    // The new connection gets its own strand
    m_acceptor.async_accept(
        net::make_strand(m_ioc),
        beast::bind_front_handler(
            &Listener::OnAccept,
            shared_from_this()
        )
    );
}

// ====================================================================================================
// HTTPSession
// ====================================================================================================
HTTPSession::HTTPSession(UI* ui, tcp::socket&& socket) :
    m_ui(ui),
    m_stream(std::move(socket))
{}

void HTTPSession::Run()
{
    DoRead();
}

// Report a failure
void HTTPSession::Fail(beast::error_code ec, char const* what)
{
    // Don't report on canceled operations
    if (ec == net::error::operation_aborted)
        return;

    std::cerr << what << ": " << ec.message() << "\n";
}

void HTTPSession::DoRead()
{
    // Construct a new parser for each message
    m_parser.emplace();

    // Apply a reasonable limit to the allowed size
    // of the body in bytes to prevent abuse.
    m_parser->body_limit(10000);

    // Set the timeout.
    m_stream.expires_after(std::chrono::seconds(30));

    // Read a request
    http::async_read(
        m_stream,
        m_buffer,
        m_parser->get(),
        beast::bind_front_handler(
            &HTTPSession::OnRead,
            shared_from_this()));
}

void HTTPSession::OnRead(beast::error_code ec, std::size_t)
{
    // This means they closed the connection
    if (ec == http::error::end_of_stream)
    {
        m_stream.socket().shutdown(tcp::socket::shutdown_send, ec);
        return;
    }

    // Handle the error, if any
    if (ec)
        return Fail(ec, "read");

    // See if it is a WebSocket Upgrade
    if (websocket::is_upgrade(m_parser->get()))
    {
        // Create a websocket session, transferring ownership
        // of both the socket and the HTTP request.
        boost::shared_ptr<WebSocketSession> session = boost::make_shared<WebSocketSession>(m_ui, m_stream.release_socket());
        UI::SetWebSocketSession(session);
        session->Run(m_parser->release());
        return;
    }

    // Handle request
    auto docRoot = "."; // Could put this in some state variable if we think it is ever worth changing, but right not it will always be "."
    http::message_generator msg = handle_request(docRoot, m_parser->release());

    // Determine if we should close the connection
    bool keep_alive = msg.keep_alive();

    auto self = shared_from_this();

    // Send the response
    beast::async_write(
        m_stream, std::move(msg),
        [self, keep_alive](beast::error_code ec, std::size_t bytes)
        {
            self->OnWrite(ec, bytes, keep_alive);
        });
}

void HTTPSession::OnWrite(beast::error_code ec, std::size_t, bool keep_alive)
{
    // Handle the error, if any
    if (ec)
        return Fail(ec, "write");

    if (!keep_alive)
    {
        // This means we should close the connection, usually because
        // the response indicated the "Connection: close" semantic.
        m_stream.socket().shutdown(tcp::socket::shutdown_send, ec);
        return;
    }

    // Read another request
    DoRead();
}


// ====================================================================================================
// WebSocketSession
// ====================================================================================================
WebSocketSession::WebSocketSession(UI* ui, tcp::socket&& socket) :
    m_ui(ui),
    m_ws(std::move(socket))
{}

WebSocketSession::~WebSocketSession()
{

}

void WebSocketSession::Fail(beast::error_code ec, char const* what)
{
    // Don't report these
    if (ec == net::error::operation_aborted ||
        ec == websocket::error::closed)
        return;

    std::cerr << what << ": " << ec.message() << "\n";
}

void WebSocketSession::OnAccept(beast::error_code ec)
{
    // Handle the error, if any
    if (ec)
        return Fail(ec, "accept");

    // Read a message
    m_ws.async_read(
        m_buffer,
        beast::bind_front_handler(
            &WebSocketSession::OnRead,
            shared_from_this()));
}

void WebSocketSession::OnRead(beast::error_code ec, std::size_t)
{
    // Handle the error, if any
    if (ec)
        return Fail(ec, "read");

    // Send the message to the UI so it can be handled
    m_ui->HandleMessage(beast::buffers_to_string(m_buffer.data()));

    // Clear the buffer
    m_buffer.consume(m_buffer.size());

    // Read another message
    m_ws.async_read(
        m_buffer,
        beast::bind_front_handler(
            &WebSocketSession::OnRead,
            shared_from_this()));
}

void WebSocketSession::Send(const std::string& msg)
{
    // Post our work to the strand, this ensures
    // that the members of `this` will not be
    // accessed concurrently.

    net::post(
        m_ws.get_executor(),
        beast::bind_front_handler(
            &WebSocketSession::OnSend,
            shared_from_this(),
            msg
        )
    );
}

void WebSocketSession::OnSend(const std::string& msg)
{
    std::cout << "Incoming: " << msg << std::endl;

    // Always add to queue
    //m_queue.push_back(ss);
    //m_queue.push_back(_s);
    m_queue.push_back(msg);

    // Are we already writing?
    if (m_queue.size() > 1)
        return;

    // We are not currently writing, so send this immediately
    m_ws.async_write(
        net::buffer(m_queue.front()),
        beast::bind_front_handler(
            &WebSocketSession::OnWrite,
            shared_from_this()));
}

void WebSocketSession::OnWrite(beast::error_code ec, std::size_t)
{
    // Handle the error, if any
    if (ec)
        return Fail(ec, "write");

    // Remove the string from the queue
    m_queue.erase(m_queue.begin());

    // Send the next message if any
    if (!m_queue.empty())
    {
        m_ws.async_write(
            net::buffer(m_queue.front()),
            beast::bind_front_handler(
                &WebSocketSession::OnWrite,
                shared_from_this()
            )
        );
    }
}
}