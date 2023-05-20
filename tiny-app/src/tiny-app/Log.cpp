#include "tiny-app-pch.h"
#include "Log.h"

#include <iostream>
#include <chrono>

namespace tiny
{
	namespace log
	{
		std::string app_current_time_and_date()
		{
			try
			{
				auto const time = std::chrono::current_zone()->to_local(std::chrono::system_clock::now());
				return std::format("{:%X}", time);
			}
			catch (const std::runtime_error&)
			{
				return "Caught runtime error";
			}
		}

		void app_core_error(const std::string& msg) noexcept
		{
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 4);
			std::cout << "[CORE " << app_current_time_and_date() << "] " << msg << std::endl;
		}
		void app_core_warn(const std::string& msg) noexcept
		{
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 6);
			std::cout << "[CORE " << app_current_time_and_date() << "] " << msg << std::endl;
		}
		void app_core_info(const std::string& msg) noexcept
		{
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
			std::cout << "[CORE " << app_current_time_and_date() << "] " << msg << std::endl;
		}
		void app_core_trace(const std::string& msg) noexcept
		{
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
			std::cout << "[CORE " << app_current_time_and_date() << "] " << msg << std::endl;
		}

	}
}