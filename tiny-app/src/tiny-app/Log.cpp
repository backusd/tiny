#include "tiny-app-pch.h"
#include "Log.h"

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

		void error(const std::string& msg) noexcept
		{
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 4);
			std::cout << "[" << current_time_and_date() << "] " << msg << std::endl;
		}
		void warn(const std::string& msg) noexcept
		{
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 6);
			std::cout << "[" << current_time_and_date() << "] " << msg << std::endl;
		}
		void info(const std::string& msg) noexcept
		{
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
			std::cout << "[" << current_time_and_date() << "] " << msg << std::endl;
		}
		void trace(const std::string& msg) noexcept
		{
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
			std::cout << "[" << current_time_and_date() << "] " << msg << std::endl;
		}
	}
}