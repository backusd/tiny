#pragma once
#include "facade-pch.h"
#include "beast.h"
#include "net.h"
#include <boost/smart_ptr.hpp>


namespace facade
{
class UI;

class Listener : public boost::enable_shared_from_this<Listener>
{
public:
    Listener(UI* ui, net::io_context& ioc, tcp::endpoint endpoint);

    // Start accepting incoming connections
    void Run();

private:
    net::io_context& m_ioc;
    tcp::acceptor m_acceptor;
    UI* m_ui;

    void Fail(beast::error_code ec, char const* what);
    void OnAccept(beast::error_code ec, tcp::socket socket);
};
}