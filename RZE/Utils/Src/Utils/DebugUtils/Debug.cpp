#include <StdAfx.h>
#include <Utils/DebugUtils/Debug.h>

#include <atomic>
#include <crtdbg.h>
#include <cstdio>
#include <ctime>
#include <iomanip>
#include <mutex>
#include <Windows.h>

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

	static std::atomic<LogSink> g_logSink = nullptr;

	void SetLogSink(LogSink sink)
	{
		g_logSink = sink;
	}

	void WriteToLog(const std::string_view str)
	{
		WriteToLogFile(str);

		if (LogSink sink = g_logSink)
		{
			std::string_view line = str;
			while (!line.empty() && (line.back() == '\n' || line.back() == '\r'))
			{
				line.remove_suffix(1);
			}

			sink(line);
		}
	}

	bool OpenConsole()
	{
		if (GetConsoleWindow() != nullptr)
		{
			return true;
		}

		if (!AllocConsole())
		{
			return false;
		}

		// A windowed app's stdout and stderr go nowhere until they're reopened on the new console
		FILE* stream = nullptr;
		freopen_s(&stream, "CONOUT$", "w", stdout);
		freopen_s(&stream, "CONOUT$", "w", stderr);
		return true;
	}

#ifdef _DEBUG
	namespace
	{
		// Writes an assert's report to stderr and the log. An assert that fires while another is being logged (e.g.
		// from the log sink) only goes to stderr, since logging it would recurse or block on the log file's lock.
		void LogAssertReport(const std::string& report)
		{
			static thread_local bool s_bIsLogging = false;

			fputs(report.c_str(), stderr);
			if (!s_bIsLogging)
			{
				s_bIsLogging = true;
				WriteToLog(report);
				s_bIsLogging = false;
			}
		}
	}

	bool ReportAssertFailure(const char* file, int line, const char* expr, const char* msg)
	{
		// The CRT dialog already prefixes the text with "Expression: ".
		std::string text = expr;
		if (msg != nullptr)
		{
			text += "\n\nMessage: ";
			text += msg;
		}

		// One log entry per assert, so the log sink runs once for it
		std::ostringstream report;
		report << "Assertion failed: " << expr << ", file " << file << ", line " << line << "\n";
		if (msg != nullptr)
		{
			report << "    Message: " << msg << "\n";
		}
		LogAssertReport(report.str());

		// _CRT_ASSERT reports go to a message box by default for both console and
		// windows subsystem apps, unlike assert() which only uses one for the latter.
		// Retry returns 1, Ignore returns 0, Abort terminates; -1 means the report
		// itself failed, so break rather than silently continuing.
		const int result = _CrtDbgReport(_CRT_ASSERT, file, line, nullptr, "%s", text.c_str());
		return result != 0;
	}
#endif
}