#pragma once

#include <queue>
#include <vector>

#include <Perseus/Threading.h>
#include <Perseus/JobSystem/WorkerThread.h>

#include <Utils/Functor.h>

namespace Perseus
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
		WorkerThread mWorkerThreads[PERSEUS_MAX_WORKER_THREADS];
		JobQueue mJobQueue;

		bool bShouldWait;
	};
}
