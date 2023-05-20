#pragma once
#include "tiny-app-pch.h"
#include "Core.h"


namespace tiny
{
	namespace log
	{
		void TINY_APP_API app_core_error(const std::string& msg) noexcept;
		void TINY_APP_API app_core_warn(const std::string& msg) noexcept;
		void TINY_APP_API app_core_info(const std::string& msg) noexcept;
		void TINY_APP_API app_core_trace(const std::string& msg) noexcept;
	}
}



#define LOG_APP_ERROR(fmt, ...) tiny::log::app_core_error(std::format(fmt, __VA_ARGS__))
#define LOG_APP_WARN(fmt, ...) tiny::log::app_core_warn(std::format(fmt, __VA_ARGS__))
#define LOG_APP_INFO(fmt, ...) tiny::log::app_core_info(std::format(fmt, __VA_ARGS__))
#define LOG_APP_TRACE(fmt, ...) tiny::log::app_core_trace(std::format(fmt, __VA_ARGS__))

