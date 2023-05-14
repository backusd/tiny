#pragma once
#include "pch.h"
#include "tiny-app/Core.h"

namespace tiny
{
// NOTE: All exception classes MUST be defined in header files ONLY. This allows us to not have to __declspec(dllexport)
//       the class. This is important because you cannot DLL export std::exception. Therefore, doing it this way, it is
//		 up to the client code to supply and link to their implementation of std::exception, which is good because then 
//       there is no dependence on a specific standard library version/implementation.
class TinyException : public std::exception
{
public:
	TinyException(int line, const char* file) noexcept :
		m_line(line),
		m_file(file)
	{}
	TinyException(const TinyException&) = delete;
	TinyException& operator=(const TinyException&) = delete;
	virtual ~TinyException() noexcept override {}

	ND virtual const char* what() const noexcept override
	{
		m_whatBuffer = std::format("{}\n{}", GetType(), GetOriginString());
		return m_whatBuffer.c_str();
	}

	ND virtual const char* GetType() const noexcept
	{
		return "Tiny Exception";
	}
	ND int GetLine() const noexcept
	{
		return m_line;
	}
	ND const std::string& GetFile() const noexcept
	{
		return m_file;
	}
	ND std::string GetOriginString() const noexcept
	{
		std::string s = std::format("[File] {}\n[Line] {}\n", m_file, m_line);
		return s;
	}

private:
	int m_line;
	std::string m_file;

protected:
	mutable std::string m_whatBuffer;
};
}