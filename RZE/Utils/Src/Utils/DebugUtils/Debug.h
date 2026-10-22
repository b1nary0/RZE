#pragma once

#include <sstream>
#include <assert.h>

#include <Utils/Platform/File.h>

#include <Utils/Platform/Timers/HiResTimer.h>

namespace Debug
{
	bool CreateLogFile();
	File& GetLogFile();
	void WriteToLogFile(const std::string_view str);

	// How much a log line matters. Debug is for high-volume detail (e.g. a line per resource), Info for start-up,
	// shutdown and one-off status, Warning for something that recovered or fell back, Error for something that failed.
	enum class ELogLevel
	{
		Debug,
		Info,
		Warning,
		Error
	};

	// Receives each logged line (without its trailing newline or level prefix). Called from whichever thread logged it.
	using LogSink = void(*)(ELogLevel level, std::string_view line);
	void SetLogSink(LogSink sink);

	// Writes to the log file with the level's prefix (e.g. "[Warning] "), then forwards to the log sink if one is set.
	void WriteToLog(ELogLevel level, const std::string_view str);

	// Writes to stdout with the level's prefix, then to the log (WriteToLog). What the RZE_LOG macros call.
	void Log(ELogLevel level, const std::string_view str);

	// Opens a console window for a windowed app and points stdout and stderr at it. Does nothing if the process
	// already has a console. Returns false if one couldn't be opened.
	bool OpenConsole();

	// Shows the CRT assert dialog (regardless of console/windows subsystem).
	// Returns true if the caller should break into the debugger.
	bool ReportAssertFailure(const char* file, int line, const char* expr, const char* msg);
}

#ifdef _DEBUG

// __debugbreak lives in the macro so the debugger stops at the failing line.
#define RZE_ASSERT_IMPL(Cond, ExprStr, Msg)										\
	do {																		\
		if (!(Cond))															\
		{																		\
			if (Debug::ReportAssertFailure(__FILE__, __LINE__, ExprStr, Msg))	\
			{																	\
				__debugbreak();													\
			}																	\
		}																		\
	} while (0)

#define AssertFalse() RZE_ASSERT_IMPL(false, "false", nullptr)
#define AssertEqual(Value1, Value2) RZE_ASSERT_IMPL((Value1) == (Value2), #Value1 " == " #Value2, nullptr)
#define AssertIsNull(Value) RZE_ASSERT_IMPL((Value) == nullptr, #Value " == nullptr", nullptr)
#define AssertNotNull(Value) RZE_ASSERT_IMPL((Value) != nullptr, #Value " != nullptr", nullptr)
#define AssertExpr(Expr) RZE_ASSERT_IMPL(Expr, #Expr, nullptr)
#define AssertMsg(Expr, Msg) RZE_ASSERT_IMPL(Expr, #Expr, Msg)

#define START_TIMED_BLOCK(Name)	\
	ScopedHiResTimer timer(Name);\

#define IMGUI_ENABLED

#else

#define AssertFalse()
#define AssertEqual(Value1, Value2)
#define AssertIsNull(Value)
#define AssertNotNull(Value)
#define AssertExpr(Expr)
#define AssertMsg(Expr, Msg)

#endif

// Logging is available in Debug and Release; only Retail strips it.
#ifndef RZE_RETAIL
#define RZE_LOGGING_ENABLED
#endif

// Each level has a stream form, RZE_LOG_<LEVEL>(Msg), e.g. RZE_LOG_WARNING("Loaded " << count << " meshes"), and a printf
// form, RZE_LOG_<LEVEL>_ARGS(Msg, ...), up to 1024 characters. RZE_LOG and RZE_LOG_ARGS log at Info.
#ifdef RZE_LOGGING_ENABLED

#define RZE_LOG_AT(Level, Msg)								\
{															\
	std::stringstream ss;									\
	ss << Msg;												\
	ss << "\n";												\
															\
	Debug::Log(Level, ss.str());							\
}

#define RZE_LOG_ARGS_AT(Level, Msg, ...)					\
{															\
	std::stringstream ss;									\
	ss << Msg;											    \
	ss << "\n";												\
															\
	char buf[1024];											\
	sprintf_s(buf, 1024, ss.str().c_str(), __VA_ARGS__);	\
	Debug::Log(Level, buf);									\
}

#else

namespace Debug
{
	template <typename... Args>
	inline void DiscardLogArgs(const Args&...) {}
}

// The args are passed to a dead branch so variables that only feed a log line don't warn as unused.
#define RZE_LOG_AT(Level, Msg) do {} while (0)
#define RZE_LOG_ARGS_AT(Level, Msg, ...) do { if (false) { Debug::DiscardLogArgs(__VA_ARGS__); } } while (0)

#endif

#define RZE_LOG_DEBUG(Msg) RZE_LOG_AT(Debug::ELogLevel::Debug, Msg)
#define RZE_LOG(Msg) RZE_LOG_AT(Debug::ELogLevel::Info, Msg)
#define RZE_LOG_WARNING(Msg) RZE_LOG_AT(Debug::ELogLevel::Warning, Msg)
#define RZE_LOG_ERROR(Msg) RZE_LOG_AT(Debug::ELogLevel::Error, Msg)

#define RZE_LOG_DEBUG_ARGS(Msg, ...) RZE_LOG_ARGS_AT(Debug::ELogLevel::Debug, Msg, __VA_ARGS__)
#define RZE_LOG_ARGS(Msg, ...) RZE_LOG_ARGS_AT(Debug::ELogLevel::Info, Msg, __VA_ARGS__)
#define RZE_LOG_WARNING_ARGS(Msg, ...) RZE_LOG_ARGS_AT(Debug::ELogLevel::Warning, Msg, __VA_ARGS__)
#define RZE_LOG_ERROR_ARGS(Msg, ...) RZE_LOG_ARGS_AT(Debug::ELogLevel::Error, Msg, __VA_ARGS__)
