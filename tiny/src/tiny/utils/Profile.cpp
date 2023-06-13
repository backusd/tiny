#include "tiny-pch.h"
#include "Profile.h"

namespace tiny
{
std::string Instrumentor::SessionName() const noexcept
{
	if (m_currentSession != nullptr)
		return m_currentSession->name;

	return "";
}

void Instrumentor::CaptureFrames(unsigned int frameCount, const std::string& name, const std::string& filepath) noexcept
{
	m_capturingFrames = true;
	m_remainingFrames = frameCount;

	// Don't begin session, wait until the start of the next frame
	//BeginSession(name, filepath);
	m_name = name;
	m_filepath = filepath;
}

void Instrumentor::CaptureSeconds(unsigned int seconds, const std::string& name, const std::string& filepath) noexcept
{
	m_capturingSeconds = true;

	m_capturingEndTime = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now()).time_since_epoch().count();
	m_capturingEndTime += (1000 * seconds);

	m_name = name;
	m_filepath = filepath;
}

void Instrumentor::NotifyNextFrame() noexcept
{
	if (SessionIsActive())
	{
		// If we are capturing frames, see if we need to end the session
		if (m_capturingFrames)
		{
			--m_remainingFrames;

			if (m_remainingFrames == 0)
			{
				m_capturingFrames = false;
				EndSession();
			}
		}
		else if (m_capturingSeconds)
		{
			long long current = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now()).time_since_epoch().count();
			if (current > m_capturingEndTime)
			{
				m_capturingSeconds = false;
				EndSession();
			}
		}
	}
	else if (m_capturingFrames || m_capturingSeconds)
	{
		// If there is no active session, but we need to start capturing frames, begin a new session
		BeginSession();
	}
}

void Instrumentor::BeginSession() noexcept
{
	m_dataCount = 0;
	m_filepath = m_filepath;
	m_currentSession = new InstrumentationSession{ m_name };
}
void Instrumentor::BeginSession(const std::string& name, std::string filepath) noexcept
{
	m_dataCount = 0;
	m_filepath = filepath;
	m_currentSession = new InstrumentationSession{ name };
}

void Instrumentor::EndSession() noexcept
{
	std::ofstream outFile(m_filepath);

	// Header
	outFile << "{\"otherData\": {},\"traceEvents\":[";

	// Data
	for (unsigned int iii = 0; iii < m_dataCount; ++iii)
	{
		if (iii != 0)
			outFile << ",";

		outFile << "{";
		outFile << "\"cat\":\"function\",";
		outFile << "\"dur\":" << (m_data[iii].end - m_data[iii].start) << ",";
		outFile << "\"name\":\"" << m_data[iii].name << "\",";
		outFile << "\"ph\":\"X\",";
		outFile << "\"pid\":0,";
		outFile << "\"tid\":" << m_data[iii].threadID << ",";
		outFile << "\"ts\":" << m_data[iii].start;
		outFile << "}";
	}

	// Footer
	outFile << "]}";

	outFile.close();

	delete m_currentSession;
	m_currentSession = nullptr;
}

void Instrumentor::WriteProfile(const std::string& name, long long start, long long end, uint32_t threadID) noexcept
{
	if (m_dataCount < 999999)
	{
		m_data[m_dataCount].name = name;
		m_data[m_dataCount].start = start;
		m_data[m_dataCount].end = end;
		m_data[m_dataCount].threadID = threadID;

		++m_dataCount;
	}
}

// -----------------------------------------------------------------------
// InstrumentationTimer
// -----------------------------------------------------------------------

InstrumentationTimer::InstrumentationTimer(const char* name) noexcept :
	m_name(name)
{
	// Don't do anything if session is not active
	if (Instrumentor::Get().SessionIsActive())
	{
		// Perform string processing before starting the timer
		//		Remove __cdecl
		size_t position = m_name.find("__cdecl ");
		if (position != std::string::npos)
			m_name.erase(position, 8);
		//		Replace (void) -> ()
		position = m_name.find("(void)");
		if (position != std::string::npos)
			m_name.erase(position + 1, 4);   // just erase "void" in "(void)"
		//		Replace " -> '
		std::replace(m_name.begin(), m_name.end(), '"', '\'');

		// std::chrono::high_resolution_clock::now() is noexcept, so nothing to handle
		m_startTimePoint = std::chrono::high_resolution_clock::now();
	}
}

void InstrumentationTimer::Stop() noexcept
{
	std::chrono::time_point<std::chrono::high_resolution_clock> endTimePoint = std::chrono::high_resolution_clock::now();

	long long start = std::chrono::time_point_cast<std::chrono::microseconds>(m_startTimePoint).time_since_epoch().count();
	long long end = std::chrono::time_point_cast<std::chrono::microseconds>(endTimePoint).time_since_epoch().count();

	uint32_t threadID = static_cast<uint32_t>(std::hash<std::thread::id>{}(std::this_thread::get_id()));

	Instrumentor::Get().WriteProfile(m_name, start, end, threadID);

	//std::chrono::time_point<std::chrono::high_resolution_clock> testEndPoint = std::chrono::high_resolution_clock::now();
	//long long testEnd = std::chrono::time_point_cast<std::chrono::microseconds>(testEndPoint).time_since_epoch().count();

	//long long duration = testEnd - start;
	//double    ms = duration * 0.001;
	//OutputDebugString(std::format("{}us ({:.3f}ms)\n", duration, ms).c_str());
}
}