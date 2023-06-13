#pragma once
#include "tiny-pch.h"
#include "tiny/Core.h"

//
// View results using chrome://tracing
//

#define PROFILE 1

#ifdef PROFILE

#include <chrono>
#include <iostream>
#include <fstream>
#include <thread>

	#define PROFILE_BEGIN_SESSION(name, filepath) tiny::Instrumentor::Get().BeginSession(name, filepath)
	#define PROFILE_END_SESSION() tiny::Instrumentor::Get().EndSession()
	#define PROFILE_NEXT_FRAME() tiny::Instrumentor::Get().NotifyNextFrame()

	#define TIMER_VAR_NAME CAT(timer, __LINE__)

	#define PROFILE_SCOPE(name) tiny::InstrumentationTimer TIMER_VAR_NAME(name)
	#define PROFILE_FUNCTION() PROFILE_SCOPE(__FUNCSIG__)

#else
	#define PROFILE_BEGIN_SESSION(name, filepath)
	#define PROFILE_END_SESSION()
	#define PROFILE_NEXT_FRAME()
	#define PROFILE_SCOPE(name)
	#define PROFILE_FUNCTION()
#endif


namespace tiny
{
struct ProfileResult
{
	std::string name = "";
	long long start = 0, end = 0;
	uint32_t threadID = 0;
};

struct InstrumentationSession
{
	std::string name;
};

// -----------------------------------------------------------------------
// Instrumentor
// -----------------------------------------------------------------------
class Instrumentor
{
public:
	Instrumentor() noexcept :
		m_currentSession(nullptr),
		m_dataCount(0),
		m_remainingFrames(0),
		m_capturingFrames(false),
		m_capturingSeconds(false),
		m_capturingEndTime(0)
	{}

	bool SessionIsActive() const noexcept { return m_currentSession != nullptr; }

	std::string SessionName() const noexcept;

	void CaptureFrames(unsigned int frameCount, const std::string& name, const std::string& filepath) noexcept;
	void CaptureSeconds(unsigned int seconds, const std::string& name, const std::string& filepath) noexcept;

	void NotifyNextFrame() noexcept;

	void BeginSession() noexcept;
	void BeginSession(const std::string& name, std::string filepath = "results.json") noexcept;

	void EndSession() noexcept;

	void WriteProfile(const std::string& name, long long start, long long end, uint32_t threadID) noexcept;

	static Instrumentor& Get() noexcept
	{
		// Allocate on heap because the m_data array uses alot of memory
		static Instrumentor* instance = new Instrumentor();
		return *instance;
	}

private:
	InstrumentationSession* m_currentSession;

	std::string m_filepath, m_name;

	std::array<ProfileResult, 1000000> m_data;
	unsigned int m_dataCount;

	unsigned int m_remainingFrames;
	bool m_capturingFrames;

	bool m_capturingSeconds;
	long long m_capturingEndTime;
};

// -----------------------------------------------------------------------
// InstrumentationTimer
// -----------------------------------------------------------------------
class InstrumentationTimer
{
public:
	InstrumentationTimer(const char* name) noexcept;

	~InstrumentationTimer()
	{
		if (Instrumentor::Get().SessionIsActive())
			Stop();
	}

	void Stop() noexcept;


private:
	std::chrono::time_point<std::chrono::high_resolution_clock> m_startTimePoint;
	std::string m_name;
};


}