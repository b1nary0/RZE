#include <doctest/doctest.h>

#include <Utils/DebugUtils/Debug.h>

#include <cstdio>

// RZE_DISABLE_OPTIMIZATION is only in code while someone debugs it, so this keeps it compiling on both compilers with
// warnings as errors. Retail rejects it on purpose, so the test isn't there.
#ifdef RZE_OPTIMIZATION_SWITCH_ENABLED

namespace
{
RZE_DISABLE_OPTIMIZATION

	// A local buffer, which old MSVC toolsets warned about in a function the optimizer is off for (C4748), so a
	// toolset that brings the warning back fails here under /WX
	int SumOfDigits(int value)
	{
		char digits[16];
		const int count = std::snprintf(digits, sizeof(digits), "%d", value);
		int sum = 0;
		for (int i = 0; i < count; ++i)
		{
			sum += digits[i] - '0';
		}
		return sum;
	}

RZE_ENABLE_OPTIMIZATION
}

TEST_CASE("RZE_DISABLE_OPTIMIZATION compiles, and the functions it covers still work")
{
	CHECK(SumOfDigits(1234) == 10);
	CHECK(SumOfDigits(0) == 0);
}

#endif
