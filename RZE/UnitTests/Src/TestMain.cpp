#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest/doctest.h>

#include <RZE.h>

#include <EngineApp.h>
#include <EngineCore/Threading/MainThreadDispatcher.h>

#include <crtdbg.h>

#include <string>

namespace
{
#ifdef _DEBUG
	// Engine asserts (Debug only) and CRT errors fail the running test instead of opening the CRT dialog, so a run
	// never waits on someone to click it, e.g. in CI
	int FailTestOnReport(int reportType, char* message, int* returnValue)
	{
		if (reportType == _CRT_WARN)
		{
			return 0;
		}

		FAIL_CHECK("Assert: " << std::string(message));
		// Don't break into the debugger
		*returnValue = 0;
		return 1;
	}
#endif
}

int main(int argc, char** argv)
{
#ifdef _DEBUG
	_CrtSetReportHook2(_CRT_RPTHOOK_INSTALL, &FailTestOnReport);
#endif

	// What the engine sets up at startup that tests rely on
	Threading::MainThreadDispatcher::Get().Initialize();
	RZE_Application::RegisterEngineComponents();

	doctest::Context context(argc, argv);
	return context.run();
}
