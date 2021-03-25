#include "Logger.h"

#include <filesystem>
#include <fstream>
#include <iostream>

using namespace std;

Logger::Logger(string pcId, string renderEngine, string objectName, TimeType timeType, char separator)
{
	m_pcId = pcId;
	m_renderEngine = renderEngine;
	m_objectName = objectName;
	m_logs = vector<Log>();
	m_timeType = timeType;
	m_separator = separator;
}

Logger::~Logger()
{
}

void Logger::ExportLogFile()
{
	#pragma region create folder
	//create folder if it doesn't exist yet
	filesystem::create_directory("data");
	#pragma endregion

	ofstream file;
	file.open("data\\" + getTimeTypeName(m_timeType) + "-data-" + m_pcId + "-" + m_renderEngine + "-" + m_objectName + ".csv");

	#pragma region --- csv header ---
	file << "timestamp" << m_separator
		<< "pc-id" << m_separator
		<< "engine" << m_separator
		<< "object" << m_separator;
	switch (m_timeType)
	{
	case TimeType::rt:
		file << "fps";
		break;
	case TimeType::nrt:
		file << "spf";
		break;
	default:
		throw runtime_error("unhandled time type");
		break;
	}
	file << m_separator << "cpu";
	#pragma endregion

	//skip first logs as it's always inaccurate as the program will be in the process of booting up
	//by the time the logs are kept it should've stabilised
	for (size_t i = 0; i < m_logs.size(); i++)
	{
		if (m_logs[i].m_cpuUsage > 0 && m_logs[i].m_frames > 0)
		{
			file << '\n';
			file << m_logs[i].m_timestamp << m_separator
				<< m_logs[i].m_pcId << m_separator
				<< m_logs[i].m_renderEngine << m_separator
				<< m_logs[i].m_objectName << m_separator
				<< m_logs[i].m_frames << m_separator
				<< m_logs[i].m_cpuUsage;
		}
	}

	file.close();
}

void Logger::AddLog(Log log)
{
	m_logs.push_back(log);
}
