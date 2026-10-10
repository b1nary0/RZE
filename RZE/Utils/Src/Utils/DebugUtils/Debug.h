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

	// Receives each logged line (without its trailing newline). Called from whichever thread logged it.
	using LogSink = void(*)(std::string_view line);
	void SetLogSink(LogSink sink);

	// Writes to the log file, then forwards to the log sink if one is set.
	void WriteToLog(const std::string_view str);

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

#define RZE_LOG(Msg) 									\
{															\
	std::stringstream ss;									\
	ss << Msg;												\
	ss << "\n";												\
															\
	printf_s(ss.str().c_str());								\
	Debug::WriteToLog(ss.str());							\
}

#define LOG_CONSOLE_ANNOUNCE(Msg, ...) 						\
{															\
	std::stringstream ss;									\
	ss << "[@ ";											\
	ss << StringUtils::ExtractFileNameFromPath(__FILE__);	\
	ss << ": ";												\
	ss << __LINE__;											\
	ss << "] ";												\
	ss << Msg;												\
	ss << "\n";												\
															\
	printf_s(ss.str().c_str(), __VA_ARGS__);				\
}

#define RZE_LOG_ARGS(Msg, ...)							\
{															\
	std::stringstream ss;									\
	ss << Msg;											    \
	ss << "\n";												\
															\
	char buf[1024];											\
	sprintf_s(buf, 1024, ss.str().c_str(), __VA_ARGS__);		\
	printf_s(ss.str().c_str(), __VA_ARGS__); 				\
	Debug::WriteToLog(buf);										\
}											

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

#define RZE_LOG
#define RZE_LOG_ARGS
#define LOG_CONSOLE_ANNOUNCE

#endif