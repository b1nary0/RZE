#include <StdAfx.h>
#include <Utils/DebugUtils/Debug.h>

#include <Utils/DebugUtils/DebugPlatform.h>
#include <Utils/Platform/Platform.h>

#include <atomic>
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
		Platform::LocalTime(time, timeInfo);

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

	namespace
	{
		// What a line at this level starts with in stdout and the log file
		const char* GetLevelPrefix(ELogLevel level)
		{
			switch (level)
			{
			case ELogLevel::Debug:
				return "[Debug] ";
			case ELogLevel::Info:
				return "[Info] ";
			case ELogLevel::Warning:
				return "[Warning] ";
			case ELogLevel::Error:
				return "[Error] ";
			default:
				return "";
			}
		}
	}

	void WriteToLog(ELogLevel level, const std::string_view str)
	{
		WriteToLogFile(std::string(GetLevelPrefix(level)).append(str));

		if (LogSink sink = g_logSink)
		{
			std::string_view line = str;
			while (!line.empty() && (line.back() == '\n' || line.back() == '\r'))
			{
				line.remove_suffix(1);
			}

			sink(level, line);
		}
	}

	void Log(ELogLevel level, const std::string_view str)
	{
		// One write, so lines from other threads don't land between the prefix and the text
		const std::string line = std::string(GetLevelPrefix(level)).append(str);
		fputs(line.c_str(), stdout);
		WriteToLog(level, str);
	}

	// OpenConsole and SetAssertFailureHook are per platform (Utils/Platform/<platform>/Debug<platform>.cpp)

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
				WriteToLog(ELogLevel::Error, report);
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

		return ShowAssertReport(file, line, text, report.str());
	}
#endif
}