#include "facade-pch.h"
#include "Listener.h"
//#include "HTTPSession.h"
#include "UI.h"

namespace facade
{
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
    FACADE_ERROR("{}: {}", what, ec.message());
}

// Handle a connection
void Listener::OnAccept(beast::error_code ec, tcp::socket socket)
{
//    if (ec)
//        return Fail(ec, "accept");
//    else
//    {
//        // Launch a new session for this connection
//        boost::shared_ptr<HTTPSession> session = boost::make_shared<HTTPSession>(m_ui, std::move(socket));
//        UI::SetHTTPSession(session);
//        session->Run();
//    }
//
//    // The new connection gets its own strand
//    m_acceptor.async_accept(
//        net::make_strand(m_ioc),
//        beast::bind_front_handler(
//            &Listener::OnAccept,
//            shared_from_this()
//        )
//    );
}

}