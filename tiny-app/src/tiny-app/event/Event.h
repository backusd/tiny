#pragma once
#include "pch.h"
#include "tiny-app/Core.h"
#include "tiny-app/Log.h"

#define EVENT_FORMATTER(class_type) \
	template <>																	\
	struct std::formatter<tiny::class_type> : std::formatter<std::string> {		\
		auto format(tiny::class_type& e, std::format_context& ctx) {			\
			return formatter<std::string>::format(e.ToString(), ctx);			\
		}																		\
	};																			\

namespace tiny
{
	enum class EventType
	{
		None = 0,
		WindowCreate, WindowClose, WindowResize, WindowFocus, WindowLostFocus, WindowMoved,
		AppTick, AppUpdate, AppRender,
		Character, KeyPressed, KeyReleased,
		MouseButtonPressed, MouseButtonReleased, MouseMove, MouseScrolled, MouseButtonDoubleClick, MouseEnter, MouseLeave
	};

	enum EventCategory
	{
		None = 0,
		EventCategoryApplication = BIT(0),
		EventCategoryInput = BIT(1),
		EventCategoryKeyboard = BIT(2),
		EventCategoryCharacter = BIT(3),
		EventCategoryMouse = BIT(4),
		EventCategoryMouseButton = BIT(5)
	};

	class TINY_APP_API Event
	{
	public:
		Event() noexcept = default;
		Event(const Event&) = default;
		Event& operator=(const Event&) = default;
		virtual ~Event() noexcept {}

		virtual EventType GetEventType() const noexcept = 0;
		virtual const char* GetName() const noexcept = 0;
		virtual int GetCategoryFlags() const noexcept = 0;
		ND inline virtual std::string ToString() const noexcept { return GetName(); }

		ND inline bool IsInCategory(EventCategory category) const noexcept
		{
			return GetCategoryFlags() & category;
		}

		ND inline bool Handled() const noexcept { return m_handled; }
		inline void Handled(bool handled) noexcept { m_handled = handled; }

	protected:
		bool m_handled = false;
	};

}