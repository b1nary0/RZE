#include <HarnessStdin.h>

#include <cerrno>

#include <unistd.h>

namespace EditorHarness
{
	size_t ReadStandardInput(char* buffer, size_t capacity)
	{
		while (true)
		{
			const ssize_t count = ::read(STDIN_FILENO, buffer, capacity);
			if (count >= 0)
			{
				return static_cast<size_t>(count);
			}
			if (errno != EINTR)
			{
				return 0;
			}
		}
	}
}
