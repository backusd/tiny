#pragma once
#include "tiny-pch.h"
#include "tiny/Core.h"
#include "tiny/exception/TinyException.h"

// Not sure of a use case for this because we can always just call infoManager.Set() and even if the HRESULT is a failure, but no debug messages
// are generated, we can still create a DeviceResourcesException with an empty messages vector
//#define GFX_THROW_NOINFO(hrcall) { HRESULT hr; if( FAILED( hr = (hrcall) ) ) throw DeviceResourcesException( __LINE__,__FILE__,hr ); }

#ifdef _DEBUG

	#define INFOMAN tiny::DxgiInfoManager& infoManager = tiny::DeviceResources::GetInfoManager();
	#define GFX_EXCEPT(hr) tiny::DeviceResourcesException( __LINE__, __FILE__, hr, infoManager.GetMessages())
	#define GFX_THROW_INFO(hrcall) { HRESULT hr; INFOMAN infoManager.Set(); if( FAILED( hr = (hrcall) ) ) throw GFX_EXCEPT(hr); }
	//#define GFX_DEVICE_REMOVED_EXCEPT(hr) { INFOMAN DeviceRemovedExceptionDX11( __LINE__,__FILE__,(hr),infoManager.GetMessages() ) }
	#define GFX_THROW_INFO_ONLY(call) { INFOMAN infoManager.Set(); call; {auto v = infoManager.GetMessages(); if(!v.empty()) {throw tiny::InfoException( __LINE__,__FILE__,v);}}}

#else

	#define GFX_EXCEPT(hr) DeviceResourcesException( __LINE__,__FILE__, hr )
	#define GFX_THROW_INFO(hrcall) { HRESULT hr; if( FAILED( hr = (hrcall) ) ) std::terminate(); }
	// #define GFX_DEVICE_REMOVED_EXCEPT(hr) DeviceRemovedException( __LINE__,__FILE__, hr )
	#define GFX_THROW_INFO_ONLY(call) call;

#endif

namespace tiny
{
// NOTE: All exception classes MUST be defined in header files ONLY. This allows us to not have to __declspec(dllexport)
//       the class. This is important because you cannot DLL export std::exception. Therefore, doing it this way, it is
//		 up to the client code to supply and link to their implementation of std::exception, which is good because then 
//       there is no dependence on a specific standard library version/implementation.
class DeviceResourcesException : public TinyException
{
public:
	DeviceResourcesException(unsigned int line, const char* file, HRESULT hr, std::vector<std::string> infoMsgs = {}) noexcept :
		TinyException(line, file),
		m_hr(hr)
	{}
	DeviceResourcesException(const DeviceResourcesException&) = delete;
	DeviceResourcesException& operator=(const DeviceResourcesException&) = delete;
	virtual ~DeviceResourcesException() noexcept override {}

	ND inline const char* GetType() const noexcept override { return "Device Resources Exception"; }
	ND inline const char* what() const noexcept override
	{
		if (m_info.empty())
			m_whatBuffer = std::format("{}\n[Error Code] {:#x} ({})\n[Error String] {}\n{}", GetType(), GetErrorCode(), GetErrorCode(), TranslateErrorCode(m_hr), GetOriginString());
		else
			m_whatBuffer = std::format("{}\n[Error Code] {:#x} ({})\n[Error String] {}\n\n[Error Info]\n{}\n\n{}", GetType(), GetErrorCode(), GetErrorCode(), TranslateErrorCode(m_hr), GetErrorInfo(), GetOriginString());

		return m_whatBuffer.c_str();
	}
	ND inline HRESULT GetErrorCode() const noexcept { return m_hr; }
	ND inline std::string GetErrorInfo() const noexcept { return m_info; }

private:
	HRESULT m_hr;
	std::string m_info;
};

// ================================================================================
class DeviceRemovedException : public DeviceResourcesException
{
public:
	DeviceRemovedException(int line, const char* file, HRESULT hr, std::vector<std::string> infoMsgs = {}) noexcept :
		DeviceResourcesException(line, file, hr, infoMsgs)
	{}
	ND const char* GetType() const noexcept override
	{
		return "Device Resources Exception [Device Removed] (DXGI_ERROR_DEVICE_REMOVED)";
	}
private:
	std::string m_reason;
};

// ===============================================================================
class InfoException : public TinyException
{
public:
	InfoException(int line, const char* file, const std::vector<std::string>& infoMsgs) noexcept :
		TinyException(line, file)
	{
		// join all info messages with newlines into single string
		for (const auto& m : infoMsgs)
		{
			m_info += m;
			m_info.push_back('\n');
		}
		// remove final newline if exists
		if (!m_info.empty())
		{
			m_info.pop_back();
		}
	}
	InfoException(const InfoException&) = delete;
	InfoException& operator=(const InfoException&) = delete;
	virtual ~InfoException() noexcept override {}

	ND const char* what() const noexcept override
	{
		m_whatBuffer = std::format("{}\n\n[Error Info]\n{}\n\n{}", GetType(), GetErrorInfo(), GetOriginString());
		return m_whatBuffer.c_str();
	}
	ND const char* GetType() const noexcept override
	{
		return "Graphics Info Exception";
	}
	ND std::string GetErrorInfo() const noexcept
	{
		return m_info;
	}
private:
	std::string m_info;
};


}