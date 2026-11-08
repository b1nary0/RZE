#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest/doctest.h>

#include <RZE.h>

#include <EngineApp.h>
#include <EngineCore/Threading/MainThreadDispatcher.h>

#include <Utils/DebugUtils/Debug.h>

#include <string>

namespace
{
#ifdef RZE_ASSERTS_ENABLED
	// Engine asserts (Debug and Development), and in Windows Debug builds the CRT's errors, fail the running test
	// instead of opening a dialog, so a run never waits on someone to click it, e.g. in CI
	bool FailTestOnAssert(const char* report)
	{
		FAIL_CHECK("Assert: " << std::string(report));
		// Don't break into the debugger
		return false;
	}
#endif
}

int main(int argc, char** argv)
{
#ifdef RZE_ASSERTS_ENABLED
	Debug::SetAssertFailureHook(&FailTestOnAssert);
#endif

	// What the engine sets up at startup that tests rely on
	Threading::MainThreadDispatcher::Get().Initialize();
	RZE_Application::RegisterEngineComponents();

	doctest::Context context(argc, argv);
	return context.run();
}
