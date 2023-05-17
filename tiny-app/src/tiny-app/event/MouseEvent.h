#pragma once
#include "tiny-app-pch.h"
#include "Event.h"
#include "tiny-app/input/MouseButton.h"

namespace tiny
{
// Mouse Moved Event -----------------------------------------------------------
class TINY_APP_API MouseMoveEvent : public Event
{
public:
	MouseMoveEvent(float x, float y) noexcept : m_mouseX(x), m_mouseY(y) {}
	MouseMoveEvent(const MouseMoveEvent&) = default;
	MouseMoveEvent& operator=(const MouseMoveEvent&) = default;
	virtual ~MouseMoveEvent() noexcept override {}

	ND inline float GetX() const noexcept { return m_mouseX; }
	ND inline float GetY() const noexcept { return m_mouseY; }

	ND inline std::string ToString() const noexcept override { return std::format("MouseMoveEvent: ({}, {})", m_mouseX, m_mouseY); }

	// Event Class Category
	ND inline virtual int GetCategoryFlags() const noexcept override { return EventCategoryMouse | EventCategoryInput; }

	// Event class type
	ND static inline EventType GetStaticType() noexcept { return EventType::MouseMove; }
	ND inline virtual EventType GetEventType() const noexcept override { return GetStaticType(); }
	ND inline virtual const char* GetName() const noexcept override { return "MouseMove"; }

private:
	float m_mouseX;
	float m_mouseY;
};

// Mouse Enter Event -----------------------------------------------------------
class TINY_APP_API MouseEnterEvent : public Event
{
public:
	MouseEnterEvent() noexcept {}
	MouseEnterEvent(const MouseEnterEvent&) = default;
	MouseEnterEvent& operator=(const MouseEnterEvent&) = default;
	virtual ~MouseEnterEvent() noexcept override {}

	ND inline std::string ToString() const noexcept override { return "MouseEnterEvent"; }

	// Event Class Category
	ND inline virtual int GetCategoryFlags() const noexcept override { return EventCategoryMouse | EventCategoryInput; }

	// Event class type
	ND static inline EventType GetStaticType() noexcept { return EventType::MouseEnter; }
	ND inline virtual EventType GetEventType() const noexcept override { return GetStaticType(); }
	ND inline virtual const char* GetName() const noexcept override { return "MouseEnter"; }
};

// Mouse Leave Event -----------------------------------------------------------
class TINY_APP_API MouseLeaveEvent : public Event
{
public:
	MouseLeaveEvent() noexcept {}
	MouseLeaveEvent(const MouseLeaveEvent&) = default;
	MouseLeaveEvent& operator=(const MouseLeaveEvent&) = default;
	virtual ~MouseLeaveEvent() noexcept override {}

	ND inline std::string ToString() const noexcept override { return "MouseLeaveEvent"; }

	// Event Class Category
	ND inline virtual int GetCategoryFlags() const noexcept override { return EventCategoryMouse | EventCategoryInput; }

	// Event class type
	ND static inline EventType GetStaticType() noexcept { return EventType::MouseLeave; }
	ND inline virtual EventType GetEventType() const noexcept override { return GetStaticType(); }
	ND inline virtual const char* GetName() const noexcept override { return "MouseLeave"; }
};

// Mouse Scrolled Event -----------------------------------------------------------
class TINY_APP_API MouseScrolledEvent : public Event
{
public:
	MouseScrolledEvent(float xOffset, float yOffset, int delta) noexcept :
		m_xOffset(xOffset), m_yOffset(yOffset), m_scrollDelta(delta)
	{}
	MouseScrolledEvent(const MouseScrolledEvent&) = default;
	MouseScrolledEvent& operator=(const MouseScrolledEvent&) = default;
	virtual ~MouseScrolledEvent() noexcept override {}

	ND inline float GetX() const noexcept { return m_xOffset; }
	ND inline float GetY() const noexcept { return m_yOffset; }
	ND inline int GetScrollDelta() const noexcept { return m_scrollDelta; }

	ND inline std::string ToString() const noexcept override { return std::format("MouseScrolledEvent - scroll: {} ({}, {})", m_scrollDelta, m_xOffset, m_yOffset); }

	// Event Class Category
	ND inline virtual int GetCategoryFlags() const noexcept override { return EventCategoryMouse | EventCategoryInput; }

	// Event class type
	ND static inline EventType GetStaticType() noexcept { return EventType::MouseScrolled; }
	ND inline virtual EventType GetEventType() const noexcept override { return GetStaticType(); }
	ND inline virtual const char* GetName() const noexcept override { return "MouseScrolled"; }

private:
	float m_xOffset;
	float m_yOffset;
	int m_scrollDelta;
};

// Mouse Button Event -----------------------------------------------------------
class TINY_APP_API MouseButtonEvent : public Event
{
public:
	ND inline MOUSE_BUTTON GetMouseButton() const noexcept { return m_button; }

	ND inline float GetX() const noexcept { return m_xOffset; }
	ND inline float GetY() const noexcept { return m_yOffset; }

	// Event Class Category
	ND inline virtual int GetCategoryFlags() const noexcept override { return EventCategoryMouse | EventCategoryMouseButton | EventCategoryInput; }

protected:
	MouseButtonEvent(MOUSE_BUTTON button, float x, float y) noexcept :
		m_button(button), m_xOffset(x), m_yOffset(y)
	{}
	MouseButtonEvent(const MouseButtonEvent&) = default;
	MouseButtonEvent& operator=(const MouseButtonEvent&) = default;
	virtual ~MouseButtonEvent() noexcept override {}

	MOUSE_BUTTON m_button;
	float m_xOffset;
	float m_yOffset;
};

// Mouse Button Pressed Event -----------------------------------------------------------
class TINY_APP_API MouseButtonPressedEvent : public MouseButtonEvent
{
public:
	MouseButtonPressedEvent(MOUSE_BUTTON button, float x, float y) noexcept :
		MouseButtonEvent(button, x, y) {}
	MouseButtonPressedEvent(const MouseButtonPressedEvent&) = default;
	MouseButtonPressedEvent& operator=(const MouseButtonPressedEvent&) = default;
	virtual ~MouseButtonPressedEvent() noexcept override {}

	ND inline std::string ToString() const noexcept override { return std::format("MouseButtonPressedEvent: {}", m_button); }

	// Event class type
	ND static inline EventType GetStaticType() noexcept { return EventType::MouseButtonPressed; }
	ND inline virtual EventType GetEventType() const noexcept override { return GetStaticType(); }
	ND inline virtual const char* GetName() const noexcept override { return "MouseButtonPressed"; }
};

// Mouse Button Released Event -----------------------------------------------------------
class TINY_APP_API MouseButtonReleasedEvent : public MouseButtonEvent
{
public:
	MouseButtonReleasedEvent(MOUSE_BUTTON button, float x, float y) noexcept :
		MouseButtonEvent(button, x, y) {}
	MouseButtonReleasedEvent(const MouseButtonReleasedEvent&) = default;
	MouseButtonReleasedEvent& operator=(const MouseButtonReleasedEvent&) = default;
	virtual ~MouseButtonReleasedEvent() noexcept override {}

	ND inline std::string ToString() const noexcept override { return std::format("MouseButtonReleasedEvent: {}", m_button); }

	// Event class type
	ND static inline EventType GetStaticType() noexcept { return EventType::MouseButtonReleased; }
	ND inline virtual EventType GetEventType() const noexcept override { return GetStaticType(); }
	ND inline virtual const char* GetName() const noexcept override { return "MouseButtonReleased"; }
};

// Mouse Button Double Click Event -----------------------------------------------------------
class TINY_APP_API MouseButtonDoubleClickEvent : public MouseButtonEvent
{
public:
	MouseButtonDoubleClickEvent(MOUSE_BUTTON button, float x, float y) noexcept :
		MouseButtonEvent(button, x, y) {}
	MouseButtonDoubleClickEvent(const MouseButtonDoubleClickEvent&) = default;
	MouseButtonDoubleClickEvent& operator=(const MouseButtonDoubleClickEvent&) = default;
	virtual ~MouseButtonDoubleClickEvent() noexcept override {}

	ND inline std::string ToString() const noexcept override { return std::format("MouseButtonDoubleClickEvent: {}", m_button); }

	// Event class type
	ND static inline EventType GetStaticType() noexcept { return EventType::MouseButtonDoubleClick; }
	ND inline virtual EventType GetEventType() const noexcept override { return GetStaticType(); }
	ND inline virtual const char* GetName() const noexcept override { return "MouseButtonDoubleClick"; }
};

}


EVENT_FORMATTER(MouseMoveEvent)
EVENT_FORMATTER(MouseEnterEvent)
EVENT_FORMATTER(MouseLeaveEvent)
EVENT_FORMATTER(MouseScrolledEvent)
EVENT_FORMATTER(MouseButtonPressedEvent)
EVENT_FORMATTER(MouseButtonReleasedEvent)
EVENT_FORMATTER(MouseButtonDoubleClickEvent)
