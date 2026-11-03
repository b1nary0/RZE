#include <StdAfx.h>
#include <Utils/Platform/Process.h>

#include <Utils/Platform/Windows/WideStrings.h>

#include <process.h>
#include <Windows.h>

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
				char buffer[1024];
				GetModuleFileNameA(NULL, buffer, 1024);
				return std::string(buffer);
			}();
		return s_executablePath;
	}

	// Not _popen: the editor has no console, so cmd would open a console window that takes focus.
	bool RunScript(const std::string& scriptAbsolutePath, const OutputLineSink& onOutputLine, const StopQuery& shouldStop, int& outExitCode)
	{
		SECURITY_ATTRIBUTES pipeAttributes = {};
		pipeAttributes.nLength = sizeof(pipeAttributes);
		pipeAttributes.bInheritHandle = TRUE;

		HANDLE readPipe = nullptr;
		HANDLE writePipe = nullptr;
		if (!CreatePipe(&readPipe, &writePipe, &pipeAttributes, 0))
		{
			return false;
		}
		// Only the child's end is inherited
		SetHandleInformation(readPipe, HANDLE_FLAG_INHERIT, 0);

		STARTUPINFOW startupInfo = {};
		startupInfo.cb = sizeof(startupInfo);
		startupInfo.dwFlags = STARTF_USESTDHANDLES;
		startupInfo.hStdInput = nullptr;
		startupInfo.hStdOutput = writePipe;
		startupInfo.hStdError = writePipe;

		// /s takes the outer quotes off, so a path with spaces stays one argument
		std::wstring commandLine = L"cmd.exe /s /c \"\"" + Conversions::StringToWString(scriptAbsolutePath) + L"\"\"";

		PROCESS_INFORMATION processInfo = {};
		const BOOL bStarted = CreateProcessW(nullptr, commandLine.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &startupInfo, &processInfo);
		// Closed here so the read below ends once the child (and anything it started) exits
		CloseHandle(writePipe);
		if (!bStarted)
		{
			CloseHandle(readPipe);
			return false;
		}

		std::string pending;
		char buffer[2048];
		DWORD bytesRead = 0;
		bool bStopped = false;
		while (!bStopped && ReadFile(readPipe, buffer, sizeof(buffer), &bytesRead, nullptr) && bytesRead > 0)
		{
			pending.append(buffer, bytesRead);

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
		CloseHandle(readPipe);
		WaitForSingleObject(processInfo.hProcess, INFINITE);

		DWORD exitCode = 0;
		GetExitCodeProcess(processInfo.hProcess, &exitCode);
		CloseHandle(processInfo.hThread);
		CloseHandle(processInfo.hProcess);

		outExitCode = static_cast<int>(exitCode);
		return true;
	}

	bool Start(const std::string& executableAbsolutePath, const std::vector<std::string>& arguments)
	{
		// _spawnv joins its arguments with spaces into one command line, which the new process splits again, so each
		// is quoted to stay whole. The program's own path is its first argument.
		std::vector<std::string> quoted;
		quoted.reserve(arguments.size() + 1);
		quoted.push_back("\"" + executableAbsolutePath + "\"");
		for (const std::string& argument : arguments)
		{
			quoted.push_back("\"" + argument + "\"");
		}

		std::vector<const char*> argv;
		argv.reserve(quoted.size() + 1);
		for (const std::string& argument : quoted)
		{
			argv.push_back(argument.c_str());
		}
		argv.push_back(nullptr);

		return _spawnv(_P_NOWAIT, executableAbsolutePath.c_str(), argv.data()) != -1;
	}
}
