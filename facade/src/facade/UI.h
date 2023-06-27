#pragma once
#include "facade-pch.h"
#include "Log.h"
#include "Listener.h"

#include <concrt.h>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

namespace facade
{
//class HTTPSession;
//class WebSocketSession;

class UI
{
public:
	static void SetMsgHandler(const std::string& key, std::function<void(const json&)> func) { Get().SetMsgHandlerImpl(key, func); }

	template<typename T>
	static void SendMsg(const T& data) { Get().SendMsgImpl<T>(data); }

	static concurrency::critical_section& GetCriticalSection() { return Get().GetCriticalSectionImpl(); }

private:
	UI();
	UI(UI&&) = delete;
	UI& operator=(UI&&) = delete;
	UI(const UI&) = delete;
	UI& operator=(const UI&) = delete;
	~UI() {}

	static UI& Get() { static UI ui; return ui; }

	void StartUIThread();
	void Run();
	void SetMsgHandlerImpl(const std::string& key, std::function<void(const json&)> func);

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

	//static void SetHTTPSession(boost::shared_ptr<HTTPSession> session) { Get().SetHTTPSessionImpl(session); }
	//void SetHTTPSessionImpl(boost::shared_ptr<HTTPSession> session) { m_httpSession = session; }
	//static void SetWebSocketSession(boost::shared_ptr<WebSocketSession> session) { Get().SetWebSocketSessionImpl(session); }
	//void SetWebSocketSessionImpl(boost::shared_ptr<WebSocketSession> session) { m_webSocketSession = session; }
	//
	//// The io_context is required for all I/O
	//net::io_context m_ioc;
	//boost::shared_ptr<HTTPSession> m_httpSession;
	//boost::shared_ptr<WebSocketSession> m_webSocketSession;

	concurrency::critical_section m_criticalSection;

	std::unordered_map<std::string, std::function<void(const json&)>> m_handlers;


	friend Listener;
	//friend HTTPSession;
	//friend WebSocketSession;
};

// Specialize the string template because this is the template that will do the actual sending of data
template<>
void UI::SendMsgImpl<std::string>(const std::string& data);


}