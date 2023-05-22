#include "tiny-pch.h"
#include "Log.h"
#include "tiny/utils/StringHelper.h"

#include <iostream>
#include <chrono>

namespace tiny
{
	namespace log
	{
		std::string current_time_and_date()
		{
			auto const time = std::chrono::current_zone()->to_local(std::chrono::system_clock::now());
			return std::format("{:%X}", time);
		}

		void core_error(const std::string& msg) noexcept
		{
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 4);
			std::cout << "[CORE " << current_time_and_date() << "] " << msg << std::endl;
		}
		void core_warn(const std::string& msg) noexcept
		{
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 6);
			std::cout << "[CORE " << current_time_and_date() << "] " << msg << std::endl;
		}
		void core_info(const std::string& msg) noexcept
		{
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
			std::cout << "[CORE " << current_time_and_date() << "] " << msg << std::endl;
		}
		void core_trace(const std::string& msg) noexcept
		{
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
			std::cout << "[CORE " << current_time_and_date() << "] " << msg << std::endl;
		}

		void core_error(const std::wstring& msg) noexcept
		{
			core_error(utility::ToString(msg));
		}		
		void core_warn(const std::wstring& msg) noexcept
		{
			core_warn(utility::ToString(msg));
		}		
		void core_info(const std::wstring& msg) noexcept
		{
			core_info(utility::ToString(msg));
		}		
		void core_trace(const std::wstring& msg) noexcept
		{
			core_trace(utility::ToString(msg));
		}

		// ==========================================================================

		void log_error(const std::string& msg) noexcept
		{
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 4);
			std::cout << "[" << current_time_and_date() << "] " << msg << std::endl;
		}
		void log_warn(const std::string& msg) noexcept
		{
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 6);
			std::cout << "[" << current_time_and_date() << "] " << msg << std::endl;
		}
		void log_info(const std::string& msg) noexcept
		{
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
			std::cout << "[" << current_time_and_date() << "] " << msg << std::endl;
		}
		void log_trace(const std::string& msg) noexcept
		{
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
			std::cout << "[" << current_time_and_date() << "] " << msg << std::endl;
		}

		void log_error(const std::wstring& msg) noexcept
		{
			log_error(utility::ToString(msg));
		}
		void log_warn(const std::wstring& msg) noexcept
		{
			log_warn(utility::ToString(msg));
		}
		void log_info(const std::wstring& msg) noexcept
		{
			log_info(utility::ToString(msg));
		}
		void log_trace(const std::wstring& msg) noexcept
		{
			log_trace(utility::ToString(msg));
		}
	}
}