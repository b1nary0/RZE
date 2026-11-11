#pragma once

#include <Utils/Functor.h>

#include <string>
#include <vector>

// This program's path, and running others. Per platform: Utils/Platform/<platform>/Process<platform>.cpp.
namespace Process
{
	// The running executable's full path (found once, then kept)
	const std::string& GetExecutablePath();

	using OutputLineSink = Functor<void, const std::string&>;
	using StopQuery = Functor<bool>;

	// Runs a script (Platform::k_buildGameScript is one) on the calling thread with no console window, and passes each
	// line of its stdout and stderr, without the trailing newline, to onOutputLine. Stops reading once shouldStop
	// returns true (asked after each line), then waits for the script to exit and sets outExitCode. Returns false if
	// it couldn't be started.
	bool RunScript(const std::string& scriptAbsolutePath, const OutputLineSink& onOutputLine, const StopQuery& shouldStop, int& outExitCode);

	// Starts an executable with arguments, without waiting for it. An argument may hold spaces. Returns false if it
	// couldn't be started.
	bool Start(const std::string& executableAbsolutePath, const std::vector<std::string>& arguments);
}
