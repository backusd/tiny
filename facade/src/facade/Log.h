#pragma once
#include "facade-pch.h"
#include "Core.h"

namespace facade
{
	namespace log
	{
		void facade_core_error(const std::string& msg) noexcept;
		void facade_core_warn(const std::string& msg) noexcept;
		void facade_core_info(const std::string& msg) noexcept;
		void facade_core_trace(const std::string& msg) noexcept;
	}
}

#define FACADE_ERROR(fmt, ...) facade::log::facade_core_error(std::format(fmt, __VA_ARGS__))
#define FACADE_WARN(fmt, ...) facade::log::facade_core_warn(std::format(fmt, __VA_ARGS__))
#define FACADE_INFO(fmt, ...) facade::log::facade_core_info(std::format(fmt, __VA_ARGS__))
#define FACADE_TRACE(fmt, ...) facade::log::facade_core_trace(std::format(fmt, __VA_ARGS__))