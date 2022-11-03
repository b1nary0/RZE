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
}

#ifdef _DEBUG

#define AssertFalse() assert(false);
#define AssertEqual(Value1, Value2) assert(Value1 == Value2);
#define AssertNull(Value) assert(Value == nullptr);
#define AssertNotNull(Value) assert(Value != nullptr)
#define AssertExpr(Expr) assert(Expr);
#define AssertMsg(Expr, Msg) assert(Expr && Msg);

#define RZE_LOG(Msg) 									\
{															\
	std::stringstream ss;									\
	ss << Msg;												\
	ss << "\n";												\
															\
	printf_s(ss.str().c_str());								\
	Debug::WriteToLogFile(ss.str());							\
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
	Debug::WriteToLogFile(buf);										\
}											

#define START_TIMED_BLOCK(Name)	\
	ScopedHiResTimer timer(Name);\

#define IMGUI_ENABLED

#else

#define AssertFalse()
#define AssertEqual(Value1, Value2)
#define AssertNull(Value)
#define AssertNotNull(Value)
#define AssertExpr(Expr)
#define AssertMsg(Expr, Msg)

#define RZE_LOG
#define RZE_LOG_ARGS
#define LOG_CONSOLE_ANNOUNCE

#endif