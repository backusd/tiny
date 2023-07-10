#pragma once
// NOTE: boost/asio MUST be included first because it attempts to use winsock2, but Windows.h includes winsock which
//       causes a problem if it is included first. Also, adding SDKDKVer.h eliminates other compiler warnings as well
//		 See: https://stackoverflow.com/questions/9750344/boostasio-winsock-and-winsock-2-compatibility-issue
#include <SDKDDKVer.h>
#include <boost/asio.hpp>
namespace net = boost::asio;                    // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;               // from <boost/asio/ip/tcp.hpp>

#include <tiny.h>


#include <concrt.h>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include <boost/beast.hpp>
namespace beast = boost::beast;                 // from <boost/beast.hpp>
namespace http = beast::http;                   // from <boost/beast/http.hpp>
namespace websocket = beast::websocket;         // from <boost/beast/websocket.hpp>


#include <cstdlib> // ?? What for?


namespace facade
{
// ====================================================================================================
// Utility functions
// ====================================================================================================
// Return a reasonable mime type based on the extension of a file.
constexpr std::string_view mime_type(std::string_view path) noexcept;

// Append an HTTP rel-path to a local filesystem path.
// The returned path is normalized for the platform.
constexpr std::string path_cat(std::string_view base, std::string_view path) noexcept;

// Return a response for the given request.
// The concrete type of the response message (which depends on the request), is type-erased in message_generator.
template <class Body, class Allocator>
http::message_generator handle_request(std::string_view doc_root, http::request<Body, http::basic_fields<Allocator>>&& req) noexcept
{
    // Returns a bad request response
    auto const bad_request = [&req](std::string_view why)
    {
        http::response<http::string_body> res{http::status::bad_request, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = std::string(why);
        res.prepare_payload();
        return res;
    };

    // Returns a not found response
    auto const not_found = [&req](std::string_view target)
    {
        http::response<http::string_body> res{http::status::not_found, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = std::format("The resource '{}' was not found", target);
        res.prepare_payload();
        return res;
    };

    // Returns a server error response
    auto const server_error = [&req](std::string_view what)
    {
        http::response<http::string_body> res{http::status::internal_server_error, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = std::format("An error occurred: '{}'", what);
        res.prepare_payload();
        return res;
    };

    // Make sure we can handle the method
    if (req.method() != http::verb::get && req.method() != http::verb::head)
        return bad_request("Unknown HTTP-method");

    // Request path must be absolute and not contain "..".
    if (req.target().empty() || req.target()[0] != '/' || req.target().find("..") != beast::string_view::npos)
        return bad_request("Illegal request-target");

    // Build the path to the requested file
    std::string path = path_cat(doc_root, req.target());
    if (req.target().back() == '/')
        path.append("index.html");

    // Attempt to open the file
    beast::error_code ec;
    http::file_body::value_type body;
    body.open(path.c_str(), beast::file_mode::scan, ec);

    // Handle the case where the file doesn't exist
    if (ec == boost::system::errc::no_such_file_or_directory)
        return not_found(req.target());

    // Handle an unknown error
    if (ec)
        return server_error(ec.message());

    // Cache the size since we need it after the move
    auto const size = body.size();

    // Respond to HEAD request
    if (req.method() == http::verb::head)
    {
        http::response<http::empty_body> res{http::status::ok, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, mime_type(path));
        res.content_length(size);
        res.keep_alive(req.keep_alive());
        return res;
    }

    // Respond to GET request
    http::response<http::file_body> res{std::piecewise_construct, std::make_tuple(std::move(body)), std::make_tuple(http::status::ok, req.version())};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, mime_type(path));
    res.content_length(size);
    res.keep_alive(req.keep_alive());
    return res;
}

// forward declare all classes
class UI;
class Listener;
class HTTPSession;
class WebSocketSession;

// ====================================================================================================
// UI
// ====================================================================================================
class UI
{
public:
	static void SetMsgHandler(const std::string& key, std::function<void(const json&)> func) { Get().SetMsgHandlerImpl(key, func); }

	template<typename T>
	static void SendMsg(const T& data) { Get().SendMsgImpl<T>(data); }

	static concurrency::critical_section& GetCriticalSection() { return Get().GetCriticalSectionImpl(); }

private:
	UI() noexcept;
	UI(UI&&) = delete;
	UI& operator=(UI&&) = delete;
	UI(const UI&) = delete;
	UI& operator=(const UI&) = delete;
	~UI() noexcept {}

	static UI& Get() noexcept { static UI ui; return ui; }

	void StartUIThread() noexcept;
	void SetMsgHandlerImpl(const std::string& key, std::function<void(const json&)> func) noexcept;

	// This template method is really just a helper method that does a struct -> json conversion
	// and then sends the json string representation to the WebSocketSession (Note: the string template
	// version of this method is specialized so that it can send the string directly to the session)
	template<typename T>
	void SendMsgImpl(const T& data)
	{
		json jsonData = data;
		SendMsgImpl<std::string>(jsonData.dump());
	}

	concurrency::critical_section& GetCriticalSectionImpl() { return m_criticalSection; }


	static void HandleMessage(const std::string& message) { Get().HandleMessageImpl(message); }
	static void HandleMessage(std::string&& message) { Get().HandleMessageImpl(message); }
	void HandleMessageImpl(const std::string& message);
	void HandleMessageImpl(std::string&& message);

	static void SetHTTPSession(std::shared_ptr<HTTPSession> session) { Get().SetHTTPSessionImpl(session); }
	void SetHTTPSessionImpl(std::shared_ptr<HTTPSession> session) { m_httpSession = session; }
	static void SetWebSocketSession(std::shared_ptr<WebSocketSession> session) { Get().SetWebSocketSessionImpl(session); }
	void SetWebSocketSessionImpl(std::shared_ptr<WebSocketSession> session) { m_webSocketSession = session; }
	
	// The io_context is required for all I/O
	net::io_context m_ioc;
	std::shared_ptr<HTTPSession> m_httpSession;
	std::shared_ptr<WebSocketSession> m_webSocketSession;

	concurrency::critical_section m_criticalSection;

	std::unordered_map<std::string, std::function<void(const json&)>> m_handlers;

	friend Listener;
	friend HTTPSession;
	friend WebSocketSession;
};

// Specialize the string template because this is the template that will do the actual sending of data
template<>
void UI::SendMsgImpl<std::string>(const std::string& data);




// ====================================================================================================
// Listener
// ====================================================================================================
class Listener : public std::enable_shared_from_this<Listener>
{
public:
	Listener(UI* ui, net::io_context& ioc, tcp::endpoint endpoint) noexcept;

	// Start accepting incoming connections
    void Run() noexcept; // Just mark this noexcept for now because I can't figure out if/what async_accept will throw

private:
	net::io_context& m_ioc;
	tcp::acceptor m_acceptor;
	UI* m_ui;

	void Fail(beast::error_code ec, char const* what) const noexcept;
	void OnAccept(beast::error_code ec, tcp::socket socket);
};

// ====================================================================================================
// HTTPSession
// ====================================================================================================
class HTTPSession : public std::enable_shared_from_this<HTTPSession>
{
public:
    HTTPSession(UI* ui, tcp::socket&& socket) noexcept;
    ~HTTPSession() noexcept {}

    void Run();

private:
    HTTPSession(const HTTPSession&) = delete;
    HTTPSession& operator=(const HTTPSession&) = delete;
    HTTPSession(HTTPSession&&) = delete;
    HTTPSession& operator=(HTTPSession&&) = delete;
    


    void Fail(beast::error_code ec, char const* what) const noexcept;
    void DoRead();
    void OnRead(beast::error_code ec, std::size_t);
    void OnWrite(beast::error_code ec, std::size_t, bool close);

    beast::tcp_stream m_stream;
    beast::flat_buffer m_buffer;
    UI* m_ui;

    // The parser is stored in an optional container so we can
    // construct it from scratch it at the beginning of each new message.
    std::optional<http::request_parser<http::string_body>> m_parser;
};

// ====================================================================================================
// WebSocketSession
// ====================================================================================================
class WebSocketSession : public std::enable_shared_from_this<WebSocketSession>
{
public:
    WebSocketSession(UI* ui, tcp::socket&& socket) noexcept;
    ~WebSocketSession() noexcept {}

    template<class Body, class Allocator>
    void Run(http::request<Body, http::basic_fields<Allocator>> req);

    // Send a message
    void Send(const std::string& msg);

private:
    WebSocketSession(const WebSocketSession&) = delete;
    WebSocketSession& operator=(const WebSocketSession&) = delete;
    WebSocketSession(WebSocketSession&&) = delete;
    WebSocketSession& operator=(WebSocketSession&&) = delete;
    

    void OnSend(const std::string& msg);

    void Fail(beast::error_code ec, char const* what) const noexcept;
    void OnAccept(beast::error_code ec);
    void OnRead(beast::error_code ec, std::size_t bytes_transferred);
    void OnWrite(beast::error_code ec, std::size_t bytes_transferred);

    beast::flat_buffer m_buffer;
    websocket::stream<beast::tcp_stream> m_ws;
    std::vector<std::string> m_queue;
    UI* m_ui;
};

template<class Body, class Allocator>
void WebSocketSession::Run(http::request<Body, http::basic_fields<Allocator>> req)
{
    // Set suggested timeout settings for the websocket
    m_ws.set_option(websocket::stream_base::timeout::suggested(beast::role_type::server));

    // Set a decorator to change the Server of the handshake
    m_ws.set_option(websocket::stream_base::decorator(
        [](websocket::response_type& res)
        {
            res.set(http::field::server,
            std::string(BOOST_BEAST_VERSION_STRING) +
            " websocket-chat-multi");
        })
    );

    // Accept the websocket handshake
    m_ws.async_accept(
        req,
        beast::bind_front_handler(
            &WebSocketSession::OnAccept,
            shared_from_this()
        )
    );
}

}