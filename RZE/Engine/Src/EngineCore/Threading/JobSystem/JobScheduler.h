#pragma once

#include <queue>
#include <vector>

#include <EngineCore/Threading/Threading.h>
#include <EngineCore/Threading/JobSystem/WorkerThread.h>

#include <Utils/Functor.h>

namespace Threading
{
	class JobScheduler
	{
	public:
		using JobQueue = std::queue<Job>;

		static std::mutex JobMutex;
	public:
		JobScheduler();
		~JobScheduler();

	public:
		void Initialize();
		void ShutDown();

	public:
		void PushJob(Job::Task task);
		bool RequestJob(Job& outJob);

		size_t GetQueueSize() { return mJobQueue.size(); }

		// #Josh(Test) :: Do not forget to deal with this.
		void Wait();

	public:
		static JobScheduler& Get()
		{
			static JobScheduler jobScheduler;
			return jobScheduler;
		}

	private:
		// The "home" array of the worker threads
		WorkerThread mWorkerThreads[MAX_WORKER_THREADS];
		JobQueue mJobQueue;

		bool bShouldWait;
	};
}
