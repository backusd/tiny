#pragma once
#include "pch.h"
#include "Core.h"


namespace tiny
{
	namespace log
	{
		void TINY_APP_API core_error(const std::string& msg) noexcept;
		void TINY_APP_API core_warn(const std::string& msg) noexcept;
		void TINY_APP_API core_info(const std::string& msg) noexcept;
		void TINY_APP_API core_trace(const std::string& msg) noexcept;
		void TINY_APP_API error(const std::string& msg) noexcept;
		void TINY_APP_API warn(const std::string& msg) noexcept;
		void TINY_APP_API info(const std::string& msg) noexcept;
		void TINY_APP_API trace(const std::string& msg) noexcept;
	}
}



#define LOG_CORE_ERROR(fmt, ...) tiny::log::core_error(std::format(fmt, __VA_ARGS__))
#define LOG_CORE_WARN(fmt, ...) tiny::log::core_warn(std::format(fmt, __VA_ARGS__))
#define LOG_CORE_INFO(fmt, ...) tiny::log::core_info(std::format(fmt, __VA_ARGS__))
#define LOG_CORE_TRACE(fmt, ...) tiny::log::core_trace(std::format(fmt, __VA_ARGS__))

#define LOG_ERROR(fmt, ...) tiny::log::error(std::format(fmt, __VA_ARGS__))
#define LOG_WARN(fmt, ...) tiny::log::warn(std::format(fmt, __VA_ARGS__))
#define LOG_INFO(fmt, ...) tiny::log::info(std::format(fmt, __VA_ARGS__))
#define LOG_TRACE(fmt, ...) tiny::log::trace(std::format(fmt, __VA_ARGS__))
