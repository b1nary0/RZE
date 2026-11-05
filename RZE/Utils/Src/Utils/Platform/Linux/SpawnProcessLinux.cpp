#include <Utils/Platform/Linux/SpawnProcessLinux.h>

#include <cerrno>
#include <csignal>
#include <thread>

#include <fcntl.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

extern char** environ;

namespace Spawn
{
	pid_t Start(const std::string& program, const std::vector<std::string>& arguments, int outputFd)
	{
		// The program's own name is its first argument
		std::vector<char*> argv;
		argv.reserve(arguments.size() + 2);
		argv.push_back(const_cast<char*>(program.c_str()));
		for (const std::string& argument : arguments)
		{
			argv.push_back(const_cast<char*>(argument.c_str()));
		}
		argv.push_back(nullptr);

		posix_spawn_file_actions_t fileActions;
		posix_spawn_file_actions_init(&fileActions);
		posix_spawn_file_actions_addopen(&fileActions, STDIN_FILENO, "/dev/null", O_RDONLY, 0);
		if (outputFd == k_discardOutput)
		{
			posix_spawn_file_actions_addopen(&fileActions, STDOUT_FILENO, "/dev/null", O_WRONLY, 0);
			posix_spawn_file_actions_adddup2(&fileActions, STDOUT_FILENO, STDERR_FILENO);
		}
		else if (outputFd != k_inheritOutput)
		{
			posix_spawn_file_actions_adddup2(&fileActions, outputFd, STDOUT_FILENO);
			posix_spawn_file_actions_adddup2(&fileActions, outputFd, STDERR_FILENO);
		}

		// The child starts with no signals blocked or ignored, whatever the calling thread has. An ignored SIGPIPE, for
		// one, would keep a child writing to a closed pipe running.
		posix_spawnattr_t attributes;
		posix_spawnattr_init(&attributes);
		sigset_t signals;
		sigemptyset(&signals);
		posix_spawnattr_setsigmask(&attributes, &signals);
		sigfillset(&signals);
		posix_spawnattr_setsigdefault(&attributes, &signals);
		posix_spawnattr_setflags(&attributes, POSIX_SPAWN_SETSIGMASK | POSIX_SPAWN_SETSIGDEF);

		pid_t pid = -1;
		const int error = posix_spawnp(&pid, program.c_str(), &fileActions, &attributes, argv.data(), environ);

		posix_spawnattr_destroy(&attributes);
		posix_spawn_file_actions_destroy(&fileActions);
		return error == 0 ? pid : -1;
	}

	int Wait(pid_t pid)
	{
		int status = 0;
		while (waitpid(pid, &status, 0) == -1)
		{
			if (errno != EINTR)
			{
				return -1;
			}
		}

		if (WIFEXITED(status))
		{
			return WEXITSTATUS(status);
		}
		if (WIFSIGNALED(status))
		{
			return 128 + WTERMSIG(status);
		}
		return -1;
	}

	void WaitInBackground(pid_t pid)
	{
		std::thread([pid]() { Wait(pid); }).detach();
	}
}
