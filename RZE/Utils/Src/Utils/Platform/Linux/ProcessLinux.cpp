#include <StdAfx.h>
#include <Utils/Platform/Process.h>

#include <Utils/Platform/Linux/SpawnProcessLinux.h>

#include <cerrno>
#include <filesystem>

#include <fcntl.h>
#include <unistd.h>

namespace
{
	void ForwardLine(std::string line, const Process::OutputLineSink& onOutputLine)
	{
		while (!line.empty() && (line.back() == '\n' || line.back() == '\r'))
		{
			line.pop_back();
		}

		onOutputLine(line);
	}
}

namespace Process
{
	const std::string& GetExecutablePath()
	{
		static const std::string s_executablePath = []()
			{
				// The kernel keeps a link to the running executable
				std::error_code error;
				return std::filesystem::read_symlink("/proc/self/exe", error).string();
			}();
		return s_executablePath;
	}

	bool RunScript(const std::string& scriptAbsolutePath, const OutputLineSink& onOutputLine, const StopQuery& shouldStop, int& outExitCode)
	{
		// Neither end is passed on to children as is: the script gets the write end as its stdout and stderr
		int pipeEnds[2];
		if (pipe2(pipeEnds, O_CLOEXEC) != 0)
		{
			return false;
		}
		const int readPipe = pipeEnds[0];
		const int writePipe = pipeEnds[1];

		// Named, as cmd.exe is on Windows, so the script runs even without its execute bit
		const pid_t pid = Spawn::Start("/bin/sh", { scriptAbsolutePath }, writePipe);
		// Closed here so the read below ends once the child (and anything it started) exits
		close(writePipe);
		if (pid == -1)
		{
			close(readPipe);
			return false;
		}

		std::string pending;
		char buffer[2048];
		bool bStopped = false;
		while (!bStopped)
		{
			const ssize_t bytesRead = read(readPipe, buffer, sizeof(buffer));
			if (bytesRead < 0 && errno == EINTR)
			{
				continue;
			}
			if (bytesRead <= 0)
			{
				break;
			}

			pending.append(buffer, static_cast<size_t>(bytesRead));

			size_t lineEnd;
			while ((lineEnd = pending.find('\n')) != std::string::npos)
			{
				ForwardLine(pending.substr(0, lineEnd), onOutputLine);
				pending.erase(0, lineEnd + 1);

				if (shouldStop())
				{
					bStopped = true;
					break;
				}
			}
		}

		if (!bStopped && !pending.empty())
		{
			ForwardLine(pending, onOutputLine);
		}

		// Like _pclose: stop reading, then wait for the process to exit
		close(readPipe);
		outExitCode = Spawn::Wait(pid);
		return true;
	}

	bool Start(const std::string& executableAbsolutePath, const std::vector<std::string>& arguments)
	{
		// Each argument reaches the program whole, so none needs quoting
		const pid_t pid = Spawn::Start(executableAbsolutePath, arguments, Spawn::k_inheritOutput);
		if (pid == -1)
		{
			return false;
		}

		// It runs on after this returns
		Spawn::WaitInBackground(pid);
		return true;
	}
}
