#include "Log.h"

#include <string>

using namespace std;

Log::Log(time_t timestamp,
	string pcId,
	string renderEngine,
	string objectName,
	float frames,
	float cpuUsage)
{
	m_timestamp = timestamp;
	m_pcId = pcId;
	m_renderEngine = renderEngine;
	m_objectName = objectName;
	m_frames = frames;
	m_cpuUsage = cpuUsage;
}

Log::~Log()
{
}
