#pragma once
#include "tiny-pch.h"
#include "Core.h"


namespace tiny
{
	namespace log
	{
		void core_error(const std::string& msg) noexcept;
		void core_warn(const std::string& msg) noexcept;
		void core_info(const std::string& msg) noexcept;
		void core_trace(const std::string& msg) noexcept;

		void core_error(const std::wstring& msg) noexcept;
		void core_warn(const std::wstring& msg) noexcept;
		void core_info(const std::wstring& msg) noexcept;
		void core_trace(const std::wstring& msg) noexcept;
	}
}



#define LOG_CORE_ERROR(fmt, ...) tiny::log::core_error(std::format(fmt, __VA_ARGS__))
#define LOG_CORE_WARN(fmt, ...) tiny::log::core_warn(std::format(fmt, __VA_ARGS__))
#define LOG_CORE_INFO(fmt, ...) tiny::log::core_info(std::format(fmt, __VA_ARGS__))
#define LOG_CORE_TRACE(fmt, ...) tiny::log::core_trace(std::format(fmt, __VA_ARGS__))
