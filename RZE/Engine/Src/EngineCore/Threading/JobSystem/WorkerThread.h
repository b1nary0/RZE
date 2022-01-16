#pragma once

#include <queue>

#include <EngineCore/Threading/Threading.h>
#include <EngineCore/Threading/JobSystem/Job.h>

namespace Threading
{
	class WorkerThread
	{
	public:
		WorkerThread();
		~WorkerThread();

	public:
		void Initialize();
		void ShutDown();

	public:
		bool IsIdle();
		bool IsRunning();

	private:
		void ThreadSetup();

	private:
		int mThreadID;

		// #TODO(Josh) These should be turned into an enum
		bool bIdle; // Sleeping, waiting for work
		bool bRunning; // false when the thread is joined
		bool bActive; // Thread is alive and either sleeping or working.

		std::queue<Job> mJobQueue;
		std::thread mThread;

	};
}