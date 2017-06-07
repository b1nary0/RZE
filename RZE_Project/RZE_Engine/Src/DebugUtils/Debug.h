#pragma once

#include <sstream>

#include "Utils/StringUtils.h"

#if DEBUG

#define AssertFalse() assert(false);
#define AssertEqual(Value1, Value2) assert(Value1 == Value2);
#define AssertNull(Value) assert(Value == nullptr);
#define AssertNotNull(Value) assert(Value != nullptr)
#define AssertExpr(Expr) assert(Expr);
#define AssertMsg(Expr, Msg) assert(Expr && Msg);

#define LOG_CONSOLE(msg) 									\
{															\
	std::stringstream ss;									\
	ss << "[@ ";											\
	ss << StringUtils::ExtractFileNameFromPath(__FILE__);	\
	ss << ": ";												\
	ss << __LINE__;											\
	ss << "] ";												\
	ss << msg;												\
	ss << "\n";												\
															\
	printf_s(ss.str().c_str());								\
}

#define LOG_CONSOLE_ANNOUNCE(msg, ...) 						\
{															\
	std::stringstream ss;									\
	ss << "[@ ";											\
	ss << StringUtils::ExtractFileNameFromPath(__FILE__);	\
	ss << ": ";												\
	ss << __LINE__;											\
	ss << "] ";												\
	ss << msg;												\
	ss << "\n";												\
															\
	printf_s(ss.str().c_str(), __VA_ARGS__);				\
}

#define LOG_CONSOLE_ARGS(msg, ...)							\
{															\
	std::stringstream ss;									\
	ss << "[@ ";											\
	ss << StringUtils::ExtractFileNameFromPath(__FILE__);	\
	ss << ": ";												\
	ss << __LINE__;											\
	ss << "] ";												\
	ss << msg;											    \
	ss << "\n";												\
															\
	printf_s(ss.str().c_str(), __VA_ARGS__); 				\
}											

#else

#define AssertFalse();
#define AssertEqual(Value1, Value2)
#define AssertNull(Value)
#define AssertNotNull(Value)
#define AssertExpr(Expr)
#define AssertMsg(Expr, Msg)

#define LOG_CONSOLE
#define LOG_CONSOLE_ARGS

#endif