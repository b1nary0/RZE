#pragma once

// What Debug.cpp needs from the platform, implemented per platform in Utils/Platform/<platform>/Debug<platform>.cpp (e.g. DebugWindows.cpp).
// Only Debug.cpp uses it.

#include <Utils/DebugUtils/DebugConfig.h>

#include <string>

namespace Debug
{
#ifdef RZE_ASSERTS_ENABLED
	// Reports a failed assert the platform's way, after Debug.cpp has logged it. text is the expression and message,
	// report the line that was logged. Returns whether to break into the debugger.
	bool ShowAssertReport(const char* file, int line, const std::string& text, const std::string& report);
#endif
}
