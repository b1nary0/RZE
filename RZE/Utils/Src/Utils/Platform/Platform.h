#pragma once

// The few things about the platform that can't be hidden behind a function: which one this is, breaking into the
// debugger, the path separator and similar constants. This and Rendering's GFXDeviceFactory.cpp are the only places
// outside vendored code that check the platform with the preprocessor. Everything else that differs per platform is a
// .cpp in a folder named for its platform (Windows/, Linux/), which the build compiles only there.

#include <ctime>

#if defined(_WIN32)
#define RZE_PLATFORM_WINDOWS 1
#elif defined(__linux__)
#define RZE_PLATFORM_LINUX 1
#include <csignal>
#else
#error "Unsupported platform"
#endif

// Stops in the debugger at the line it's written on (a function would stop inside itself)
#if RZE_PLATFORM_WINDOWS
#define RZE_DEBUG_BREAK() __debugbreak()
#elif defined(__clang__)
#define RZE_DEBUG_BREAK() __builtin_debugtrap()
#else
#define RZE_DEBUG_BREAK() raise(SIGTRAP)
#endif

namespace Platform
{
#if RZE_PLATFORM_WINDOWS
	// What separates folders in the paths Filepath builds
	constexpr char k_pathSeparator = '\\';
	// The other one, which Filepath accepts and turns into k_pathSeparator
	constexpr char k_otherPathSeparator = '/';
	constexpr const char* k_executableSuffix = ".exe";
	// The script that builds the game, in the source tree (GameBuilder)
	constexpr const char* k_buildGameScript = "BuildGame.bat";
#else
	constexpr char k_pathSeparator = '/';
	constexpr char k_otherPathSeparator = '\\';
	constexpr const char* k_executableSuffix = "";
	constexpr const char* k_buildGameScript = "BuildGame.sh";
#endif

	// time as local calendar time. (MSVC's localtime_s and POSIX's localtime_r take their arguments in opposite
	// orders, so neither works everywhere.)
	inline void LocalTime(std::time_t time, std::tm& outLocal)
	{
#if RZE_PLATFORM_WINDOWS
		localtime_s(&outLocal, &time);
#else
		localtime_r(&time, &outLocal);
#endif
	}
}
