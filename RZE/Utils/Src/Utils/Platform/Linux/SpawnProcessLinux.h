#pragma once

// Starting and waiting on child processes, for the Linux Process and Shell. Linux only.

#include <string>
#include <vector>

#include <sys/types.h>

namespace Spawn
{
	// Where a child's stdout and stderr go, besides a file descriptor of ours
	constexpr int k_inheritOutput = -1; // Where ours go
	constexpr int k_discardOutput = -2; // Nowhere (/dev/null)

	// Starts program with arguments, without waiting for it. program is looked up on PATH unless it's a path. Its stdin
	// is /dev/null, and its stdout and stderr both go to outputFd. Returns its pid, or -1 if it couldn't be started.
	pid_t Start(const std::string& program, const std::vector<std::string>& arguments, int outputFd);

	// Waits for a child to exit. Returns its exit code, 128 plus the signal's number if a signal ended it (as shells
	// report it), or -1 if it can't be waited on.
	int Wait(pid_t pid);

	// Waits for a child on a thread of its own. An exited child stays a zombie until it's waited on, so a child that's
	// left to run needs this.
	void WaitInBackground(pid_t pid);
}
