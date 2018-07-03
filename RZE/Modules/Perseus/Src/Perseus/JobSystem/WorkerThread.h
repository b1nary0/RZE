#pragma once

#include <queue>

#include <Perseus/Threading.h>
#include <Perseus/JobSystem/Job.h>

namespace Perseus
{
	class WorkerThread
	{
	public:
		WorkerThread();
		~WorkerThread();

		void Initialize();
		bool bIdle;

	private:
		void ThreadSetup();

	private:
		int mThreadID;

		std::queue<Job> mJobQueue;
		std::thread mThread;

	};
}