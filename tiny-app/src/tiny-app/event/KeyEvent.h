#pragma once
#include "pch.h"
#include "Event.h"
#include "tiny-app/input/KeyCode.h"


namespace tiny
{
// CharEvent -----------------------------------------------------
class TINY_APP_API CharEvent : public Event
{
public:
	CharEvent(char keycode, int repeatCount) noexcept : m_keycode(keycode), m_repeatCount(repeatCount) {}
	CharEvent(const CharEvent&) = default;
	CharEvent& operator=(const CharEvent&) = default;
	virtual ~CharEvent() noexcept override {}

	ND inline char GetKeyCode() const noexcept { return m_keycode; }
	ND inline int GetRepeatCount() const noexcept { return m_repeatCount; }

	ND inline std::string ToString() const noexcept override { return std::format("CharEvent: {} (repeats: {})", m_keycode, m_repeatCount); }

	// Event Class Category
	ND inline virtual int GetCategoryFlags() const noexcept override { return EventCategoryKeyboard | EventCategoryInput | EventCategoryCharacter; }

	// Event class type
	ND static inline EventType GetStaticType() noexcept { return EventType::Character; }
	ND inline virtual EventType GetEventType() const noexcept override { return GetStaticType(); }
	ND inline virtual const char* GetName() const noexcept override { return "Char"; }

protected:
	const char m_keycode;
	const int m_repeatCount;
};

// KeyEvent -----------------------------------------------------
class TINY_APP_API KeyEvent : public Event
{
public:
	ND inline KEY_CODE GetKeyCode() const noexcept { return m_keycode; }

	// Event Class Category
	ND inline virtual int GetCategoryFlags() const noexcept override { return EventCategoryKeyboard | EventCategoryInput; }

protected:
	KeyEvent(KEY_CODE keycode) noexcept : m_keycode(keycode) {}
	KeyEvent(const KeyEvent&) = default;
	KeyEvent& operator=(const KeyEvent&) = default;
	virtual ~KeyEvent() noexcept override {}

	const KEY_CODE m_keycode;
};

// KeyPressedEvent -----------------------------------------------------
class TINY_APP_API KeyPressedEvent : public KeyEvent
{
public:
	KeyPressedEvent(KEY_CODE keycode, int repeatCount, bool keyWasPreviouslyDown) noexcept :
		KeyEvent(keycode), m_repeatCount(repeatCount), m_keyWasPreviouslyDown(keyWasPreviouslyDown)
	{}
	KeyPressedEvent(const KeyPressedEvent&) = default;
	KeyPressedEvent& operator=(const KeyPressedEvent&) = default;
	virtual ~KeyPressedEvent() noexcept override {}


	ND inline int GetRepeatCount() const noexcept { return m_repeatCount; }
	ND inline bool KeyWasPreviouslyDown() const noexcept { return m_keyWasPreviouslyDown; }

	ND inline std::string ToString() const noexcept override { return std::format("KeyPressedEvent: {} (repeats: {}, previously down: {})", m_keycode, m_repeatCount, m_keyWasPreviouslyDown); }

	// Event class type
	ND static inline EventType GetStaticType() noexcept { return EventType::KeyPressed; }
	ND inline virtual EventType GetEventType() const noexcept override { return GetStaticType(); }
	ND inline virtual const char* GetName() const noexcept override { return "KeyPressed"; }

private:
	const int m_repeatCount;
	const bool m_keyWasPreviouslyDown;
};

// KeyReleasedEvent -----------------------------------------------------
class TINY_APP_API KeyReleasedEvent : public KeyEvent
{
public:
	KeyReleasedEvent(KEY_CODE keycode) noexcept : KeyEvent(keycode) {}
	KeyReleasedEvent(const KeyReleasedEvent&) = default;
	KeyReleasedEvent& operator=(const KeyReleasedEvent&) = default;
	virtual ~KeyReleasedEvent() noexcept override {}

	ND inline std::string ToString() const noexcept override { return std::format("KeyReleasedEvent: {}", m_keycode); }

	// Event class type
	ND static inline EventType GetStaticType() noexcept { return EventType::KeyReleased; }
	ND inline virtual EventType GetEventType() const noexcept override { return GetStaticType(); }
	ND inline virtual const char* GetName() const noexcept override { return "KeyReleased"; }
};


} 

EVENT_FORMATTER(CharEvent)
EVENT_FORMATTER(KeyPressedEvent)
EVENT_FORMATTER(KeyReleasedEvent)
