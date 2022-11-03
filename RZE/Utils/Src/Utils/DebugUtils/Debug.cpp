#include <Utils/DebugUtils/Debug.h>

#include <ctime>

namespace Debug
{
	static File g_logFile;

	bool CreateLogFile()
	{
		std::time_t time = std::time(nullptr);
		tm timeInfo;
		localtime_s(&timeInfo, &time);

		std::ostringstream ss;
		ss << std::put_time(&timeInfo, "%d-%m-%Y_%H-%M-%S");

		Filepath logFilePath("Logs/RZELOG_" + ss.str() + ".txt");
		g_logFile = File(logFilePath, true);
		g_logFile.Open(File::EFileOpenMode::Write);
		g_logFile.Close();

		return true;
	}

	File& GetLogFile()
	{
		return g_logFile;
	}

	void WriteToLogFile(const std::string_view str)
	{
		File& logFile = Debug::GetLogFile();
		logFile.Open(File::EFileOpenMode::Append);
		logFile << str.data();
		logFile.Close();
	}
}