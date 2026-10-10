#include <StdAfx.h>
#include <Utils/DebugUtils/Debug.h>

#include <crtdbg.h>
#include <cstdio>
#include <ctime>
#include <iomanip>
#include <mutex>

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
		// The log file is a single shared handle that gets opened/closed per write,
		// so writes from worker threads must be serialized.
		static std::mutex s_logFileMutex;
		std::lock_guard<std::mutex> lock(s_logFileMutex);

		File& logFile = Debug::GetLogFile();
		logFile.Open(File::EFileOpenMode::Append);
		logFile << str.data();
		logFile.Close();
	}

#ifdef _DEBUG
	bool ReportAssertFailure(const char* file, int line, const char* expr, const char* msg)
	{
		// The CRT dialog already prefixes the text with "Expression: ".
		std::string text = expr;
		if (msg != nullptr)
		{
			text += "\n\nMessage: ";
			text += msg;
		}

		fprintf(stderr, "Assertion failed: %s, file %s, line %d\n", expr, file, line);
		if (msg != nullptr)
		{
			fprintf(stderr, "    Message: %s\n", msg);
		}

		// _CRT_ASSERT reports go to a message box by default for both console and
		// windows subsystem apps, unlike assert() which only uses one for the latter.
		// Retry returns 1, Ignore returns 0, Abort terminates; -1 means the report
		// itself failed, so break rather than silently continuing.
		const int result = _CrtDbgReport(_CRT_ASSERT, file, line, nullptr, "%s", text.c_str());
		return result != 0;
	}
#endif
}