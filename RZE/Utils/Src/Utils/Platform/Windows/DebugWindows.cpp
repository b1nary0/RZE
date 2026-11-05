#include <Utils/DebugUtils/Debug.h>
#include <Utils/DebugUtils/DebugPlatform.h>

#include <crtdbg.h>
#include <cstdio>
#include <Windows.h>

namespace Debug
{
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

	namespace
	{
		AssertFailureHook g_assertFailureHook = nullptr;

#ifdef _DEBUG
		// The hook sees asserts (ShowAssertReport below) and the CRT's own error reports, but not its warnings
		int CallAssertFailureHook(int reportType, char* message, int* returnValue)
		{
			if (reportType == _CRT_WARN)
			{
				return 0;
			}

			*returnValue = g_assertFailureHook(message) ? 1 : 0;
			// Handled: no dialog
			return 1;
		}
#endif
	}

	void SetAssertFailureHook(AssertFailureHook hook)
	{
		// Only the debug CRT reports, so only Debug has anything to hook
#ifdef _DEBUG
		if (g_assertFailureHook != nullptr)
		{
			_CrtSetReportHook2(_CRT_RPTHOOK_REMOVE, &CallAssertFailureHook);
		}
#endif

		g_assertFailureHook = hook;

#ifdef _DEBUG
		if (g_assertFailureHook != nullptr)
		{
			_CrtSetReportHook2(_CRT_RPTHOOK_INSTALL, &CallAssertFailureHook);
		}
#endif
	}

#ifdef _DEBUG
	bool ShowAssertReport(const char* file, int line, const std::string& text, const std::string& /*report*/)
	{
		// _CRT_ASSERT reports go to a message box by default for both console and
		// windows subsystem apps, unlike assert() which only uses one for the latter.
		// Retry returns 1, Ignore returns 0, Abort terminates; -1 means the report
		// itself failed, so break rather than silently continuing.
		const int result = _CrtDbgReport(_CRT_ASSERT, file, line, nullptr, "%s", text.c_str());
		return result != 0;
	}
#endif
}
