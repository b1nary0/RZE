#pragma once

#include <sstream>
#include <assert.h>

#include <Utils/StringUtils.h>
#include <Utils/Platform/Timers/HiResTimer.h>

#ifdef _DEBUG

#define AssertFalse() assert(false);
#define AssertEqual(Value1, Value2) assert(Value1 == Value2);
#define AssertNull(Value) assert(Value == nullptr);
#define AssertNotNull(Value) assert(Value != nullptr)
#define AssertExpr(Expr) assert(Expr);
#define AssertMsg(Expr, Msg) assert(Expr && Msg);

#define LOG_CONSOLE(Msg) 									\
{															\
	std::stringstream ss;									\
	ss << Msg;												\
	ss << "\n";												\
															\
	printf_s(ss.str().c_str());								\
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

#define LOG_CONSOLE_ARGS(Msg, ...)							\
{															\
	std::stringstream ss;									\
	ss << Msg;											    \
	ss << "\n";												\
															\
	printf_s(ss.str().c_str(), __VA_ARGS__); 				\
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

#define LOG_CONSOLE
#define LOG_CONSOLE_ARGS
#define LOG_CONSOLE_ANNOUNCE

#endif