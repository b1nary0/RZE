#include <HarnessStdin.h>

#include <Windows.h>

namespace EditorHarness
{
	size_t ReadStandardInput(char* buffer, size_t capacity)
	{
		DWORD count = 0;
		if (!ReadFile(GetStdHandle(STD_INPUT_HANDLE), buffer, static_cast<DWORD>(capacity), &count, nullptr))
		{
			return 0;
		}
		return static_cast<size_t>(count);
	}
}
