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
		void Finish();
		bool bIdle;

	private:
		void ThreadSetup();

	private:
		int mThreadID;

		bool bRunning;

		std::queue<Job> mJobQueue;
		std::thread mThread;

	};
}