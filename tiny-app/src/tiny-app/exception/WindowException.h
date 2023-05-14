#pragma once
#include "pch.h"
#include "TinyException.h"
#include "tiny-app/utils/TranslateErrorCode.h"

#define WINDOW_EXCEPT( hr ) WindowException( __LINE__,__FILE__,hr )
#define WINDOW_LAST_EXCEPT() WindowException( __LINE__,__FILE__,GetLastError() )

namespace tiny
{
// NOTE: All exception classes MUST be defined in header files ONLY. This allows us to not have to __declspec(dllexport)
//       the class. This is important because you cannot DLL export std::exception. Therefore, doing it this way, it is
//		 up to the client code to supply and link to their implementation of std::exception, which is good because then 
//       there is no dependence on a specific standard library version/implementation.
class WindowException : public TinyException
{
public:
	WindowException(unsigned int line, const char* file, HRESULT hr) noexcept :
		TinyException(line, file),
		m_hr(hr)
	{}
	WindowException(const WindowException&) = delete;
	WindowException& operator=(const WindowException&) = delete;
	virtual ~WindowException() noexcept override {}

	ND const char* GetType() const noexcept override
	{
		return "Window Exception";
	}

	ND const char* what() const noexcept override
	{
		m_whatBuffer = std::format("{}\n[Error Code] {:#x} ({})\n[Description] {}\n{}", GetType(), GetErrorCode(), GetErrorCode(), GetErrorDescription(), GetOriginString());
		return m_whatBuffer.c_str();
	}
	ND HRESULT GetErrorCode() const noexcept
	{
		return m_hr;
	}
	ND std::string GetErrorDescription() const noexcept
	{
		return TranslateErrorCode(m_hr);
	}

private:
	HRESULT m_hr;
};

}