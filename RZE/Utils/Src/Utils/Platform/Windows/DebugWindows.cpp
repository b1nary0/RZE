#include <Utils/DebugUtils/Debug.h>
#include <Utils/DebugUtils/DebugPlatform.h>

#include <crtdbg.h>
#include <cstdio>
#include <cstdlib>
#include <string>
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
		// In Debug the hook goes through the debug CRT's report hook, so it also sees the CRT's own errors. The release
		// CRT has no reports, so in Development ShowAssertReport calls the hook itself.
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

#ifdef RZE_ASSERTS_ENABLED
	// report is the logged line, which only Development's hook call uses: the CRT's dialog doesn't show it
	bool ShowAssertReport(const char* file, int line, const std::string& text, [[maybe_unused]] const std::string& report)
	{
#ifdef _DEBUG
		// _CRT_ASSERT reports go to a message box by default for both console and
		// windows subsystem apps, unlike assert() which only uses one for the latter.
		// Retry returns 1, Ignore returns 0, Abort terminates; -1 means the report
		// itself failed, so break rather than silently continuing.
		const int result = _CrtDbgReport(_CRT_ASSERT, file, line, nullptr, "%s", text.c_str());
		return result != 0;
#else
		if (g_assertFailureHook != nullptr)
		{
			return g_assertFailureHook(report.c_str());
		}

		// The release CRT has no assert dialog, so this is one with the CRT's buttons and meanings
		const std::string message = "Expression: " + text + "\n\nFile: " + file + "\nLine: " + std::to_string(line)
			+ "\n\n(Press Retry to debug the application)";
		switch (MessageBoxA(nullptr, message.c_str(), "RZE: Assertion Failed",
			MB_ABORTRETRYIGNORE | MB_ICONERROR | MB_TASKMODAL | MB_SETFOREGROUND))
		{
		case IDABORT:
			std::abort();
		case IDIGNORE:
			return false;
		default:
			// Retry, or the box couldn't be shown (0): break rather than silently continuing, as the CRT does
			return true;
		}
#endif
	}
#endif
}
