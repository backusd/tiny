#pragma once
#include "tiny-app-pch.h"
#include "Event.h"

namespace tiny
{
// Window Resize Event -----------------------------------------------------------
class TINY_APP_API WindowResizeEvent : public Event
{
public:
	WindowResizeEvent(unsigned int width, unsigned int height) noexcept :
		m_width(width), m_height(height) {}
	WindowResizeEvent(const WindowResizeEvent&) = default;
	WindowResizeEvent& operator=(const WindowResizeEvent&) = default;
	virtual ~WindowResizeEvent() noexcept override {}

	ND inline unsigned int GetWidth() const noexcept { return m_width; }
	ND inline unsigned int GetHeight() const noexcept { return m_height; }

	ND inline std::string ToString() const noexcept override { return std::format("WindowResizeEvent: width = {}, height = {}", m_width, m_height); }

	// Event Class Category
	ND inline virtual int GetCategoryFlags() const noexcept override { return EventCategoryApplication; }

	// Event class type
	ND static inline EventType GetStaticType() noexcept { return EventType::WindowResize; }
	ND inline virtual EventType GetEventType() const noexcept override { return GetStaticType(); }
	ND inline virtual const char* GetName() const noexcept override { return "WindowResize"; }

private:
	const unsigned int m_width;
	const unsigned int m_height;
};

// Window Create Event -----------------------------------------------------------
class TINY_APP_API WindowCreateEvent : public Event
{
public:
	WindowCreateEvent(unsigned int top, unsigned int left, unsigned int width, unsigned int height) noexcept :
		m_top(top), m_left(left), m_width(width), m_height(height)
	{}
	WindowCreateEvent(const WindowCreateEvent&) = default;
	WindowCreateEvent& operator=(const WindowCreateEvent&) = default;
	virtual ~WindowCreateEvent() noexcept override {}

	ND inline std::string ToString() const noexcept override { return std::format("WindowCreateEvent (top: {}, left: {}, width: {}, height: {}", m_top, m_left, m_width, m_height); }

	// Event Class Category
	ND inline virtual int GetCategoryFlags() const noexcept override { return EventCategoryApplication; }

	// Event class type
	ND static inline EventType GetStaticType() noexcept { return EventType::WindowCreate; }
	ND inline virtual EventType GetEventType() const noexcept override { return GetStaticType(); }
	ND inline virtual const char* GetName() const noexcept override { return "WindowCreate"; }

private:
	const unsigned int m_top;
	const unsigned int m_left;
	const unsigned int m_width;
	const unsigned int m_height;
};

// Window Close Event -----------------------------------------------------------
class TINY_APP_API WindowCloseEvent : public Event
{
public:
	WindowCloseEvent() noexcept {}
	WindowCloseEvent(const WindowCloseEvent&) = default;
	WindowCloseEvent& operator=(const WindowCloseEvent&) = default;
	virtual ~WindowCloseEvent() noexcept override {}

	ND inline std::string ToString() const noexcept override { return "WindowCloseEvent"; }

	// Event Class Category
	ND inline virtual int GetCategoryFlags() const noexcept override { return EventCategoryApplication; }

	// Event class type
	ND static inline EventType GetStaticType() noexcept { return EventType::WindowClose; }
	ND inline virtual EventType GetEventType() const noexcept override { return GetStaticType(); }
	ND inline virtual const char* GetName() const noexcept override { return "WindowClose"; }
};

// App Tick Event -----------------------------------------------------------
class TINY_APP_API AppTickEvent : public Event
{
public:
	AppTickEvent() noexcept {}
	AppTickEvent(const AppTickEvent&) = default;
	AppTickEvent& operator=(const AppTickEvent&) = default;
	virtual ~AppTickEvent() noexcept override {}

	ND inline std::string ToString() const noexcept override { return "AppTickEvent"; }

	// Event Class Category
	ND inline virtual int GetCategoryFlags() const noexcept override { return EventCategoryApplication; }

	// Event class type
	ND static inline EventType GetStaticType() noexcept { return EventType::AppTick; }
	ND inline virtual EventType GetEventType() const noexcept override { return GetStaticType(); }
	ND inline virtual const char* GetName() const noexcept override { return "AppTick"; }
};

// App Update Event -----------------------------------------------------------
class TINY_APP_API AppUpdateEvent : public Event
{
public:
	AppUpdateEvent() noexcept {}
	AppUpdateEvent(const AppUpdateEvent&) = default;
	AppUpdateEvent& operator=(const AppUpdateEvent&) = default;
	virtual ~AppUpdateEvent() noexcept override {}

	ND inline std::string ToString() const noexcept override { return "AppUpdateEvent"; }

	// Event Class Category
	ND inline virtual int GetCategoryFlags() const noexcept override { return EventCategoryApplication; }

	// Event class type
	ND static inline EventType GetStaticType() noexcept { return EventType::AppUpdate; }
	ND inline virtual EventType GetEventType() const noexcept override { return GetStaticType(); }
	ND inline virtual const char* GetName() const noexcept override { return "AppUpdate"; }
};

// App Render Event -----------------------------------------------------------
class TINY_APP_API AppRenderEvent : public Event
{
public:
	AppRenderEvent() noexcept {}
	AppRenderEvent(const AppRenderEvent&) = default;
	AppRenderEvent& operator=(const AppRenderEvent&) = default;
	virtual ~AppRenderEvent() noexcept override {}

	ND inline std::string ToString() const noexcept override { return "AppRenderEvent"; }

	// Event Class Category
	ND inline virtual int GetCategoryFlags() const noexcept override { return EventCategoryApplication; }

	// Event class type
	ND static inline EventType GetStaticType() noexcept { return EventType::AppRender; }
	ND inline virtual EventType GetEventType() const noexcept override { return GetStaticType(); }
	ND inline virtual const char* GetName() const noexcept override { return "AppRender"; }
};

}

EVENT_FORMATTER(WindowResizeEvent)
EVENT_FORMATTER(WindowCreateEvent)
EVENT_FORMATTER(WindowCloseEvent)
EVENT_FORMATTER(AppTickEvent)
EVENT_FORMATTER(AppUpdateEvent)
EVENT_FORMATTER(AppRenderEvent)
