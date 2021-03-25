#pragma once

#include <ctime>
#include <string>

using namespace std;

class Log {
public:
	Log(time_t timestamp,
		string pcId,
		string renderEngine,
		string objectName,
		float frames,
		float cpuUsage);
	~Log();

	time_t m_timestamp;
	string m_pcId;
	string m_renderEngine;
	string m_objectName;
	float m_frames;
	float m_cpuUsage;
};
