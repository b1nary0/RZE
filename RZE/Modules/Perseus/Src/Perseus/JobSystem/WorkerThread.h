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

	public:
		void Initialize();
		void Finish();

	public:
		bool IsIdle();
		bool IsRunning();

	private:
		void ThreadSetup();

	private:
		int mThreadID;

		bool bIdle; // Sleeping, waiting for work
		bool bRunning; // false when the thread is joined
		bool bActive; // Thread is alive and either sleeping or working.

		std::queue<Job> mJobQueue;
		std::thread mThread;

	};
}