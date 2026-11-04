#pragma once

#include <cstddef>

namespace EditorHarness
{
	// Reads what stdin has, blocking until it has some; 0 at its end. It goes around the C runtime's streams, which a
	// thread blocked reading through them keeps locked, and exit flushes every stream, so the process couldn't exit.
	// Per platform: Src/<platform>/HarnessStdin<platform>.cpp.
	size_t ReadStandardInput(char* buffer, size_t capacity);
}
