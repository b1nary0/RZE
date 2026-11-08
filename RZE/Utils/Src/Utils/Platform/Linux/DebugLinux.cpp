#include <Utils/DebugUtils/Debug.h>
#include <Utils/DebugUtils/DebugPlatform.h>

#include <fstream>
#include <string>

namespace Debug
{
	bool OpenConsole()
	{
		// Linux has no console and windowed kinds of program: stdout and stderr already go wherever it was started from
		return true;
	}

	namespace
	{
#ifdef RZE_ASSERTS_ENABLED
		AssertFailureHook g_assertFailureHook = nullptr;

		bool IsDebuggerAttached()
		{
			// The kernel names the process tracing this one, 0 for none
			std::ifstream status("/proc/self/status");
			std::string line;
			while (std::getline(status, line))
			{
				if (line.rfind("TracerPid:", 0) == 0)
				{
					return line.find_first_not_of(" \t0", sizeof("TracerPid:") - 1) != std::string::npos;
				}
			}
			return false;
		}
#endif
	}

	void SetAssertFailureHook(AssertFailureHook hook)
	{
		// Asserts compile out in Retail, so there's nothing to hook there
#ifdef RZE_ASSERTS_ENABLED
		g_assertFailureHook = hook;
#endif
	}

#ifdef RZE_ASSERTS_ENABLED
	bool ShowAssertReport(const char* /*file*/, int /*line*/, const std::string& /*text*/, const std::string& report)
	{
		if (g_assertFailureHook != nullptr)
		{
			return g_assertFailureHook(report.c_str());
		}

		// No dialog to ask, so: break where a debugger will catch it, as Retry does, and otherwise carry on, as Ignore
		// does. The report is already in stderr and the log.
		return IsDebuggerAttached();
	}
#endif
}
