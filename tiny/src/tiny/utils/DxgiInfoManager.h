#pragma once
#include "tiny-pch.h"
#include "tiny/Core.h"
#include "tiny/exception/TinyException.h"
#include "tiny/utils/TranslateErrorCode.h"


namespace tiny
{
class DxgiInfoManagerException : public TinyException
{
public:
	DxgiInfoManagerException(unsigned int line, const char* file, HRESULT hr) noexcept :
		TinyException(line, file),
		m_hr(hr)
	{}
	DxgiInfoManagerException(const DxgiInfoManagerException&) = delete;
	DxgiInfoManagerException& operator=(const DxgiInfoManagerException&) = delete;
	virtual ~DxgiInfoManagerException() noexcept override {}

	ND const char* GetType() const noexcept override { return "DxgiInfoManager Exception"; }
	ND const char* what() const noexcept override
	{
		m_whatBuffer = std::format("{}\n[Error Code] {:#x} ({})\n[Description] {}\n{}", GetType(), GetErrorCode(), GetErrorCode(), GetErrorDescription(), GetOriginString());
		return m_whatBuffer.c_str();
	}
	ND HRESULT GetErrorCode() const noexcept { return m_hr; }
	ND std::string GetErrorDescription() const noexcept { return TranslateErrorCode(m_hr); }

private:
	HRESULT m_hr;
};

// ==============================================================================================================

#pragma warning( push )
#pragma warning( disable : 4251 ) // needs to have dll-interface to be used by clients of class
class TINY_API DxgiInfoManager
{
public:
	DxgiInfoManager();
	DxgiInfoManager(const DxgiInfoManager&) = delete;
	DxgiInfoManager& operator=(const DxgiInfoManager&) = delete;
	~DxgiInfoManager() noexcept;
	void Set() noexcept;
	std::vector<std::string> GetMessages() const;
private:
	unsigned long long next = 0u;
	struct IDXGIInfoQueue* pDxgiInfoQueue = nullptr;
};
#pragma warning( pop )
}