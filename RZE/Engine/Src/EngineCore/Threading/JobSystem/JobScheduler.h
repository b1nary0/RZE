#pragma once

#include <queue>

#include <EngineCore/Threading/Threading.h>
#include <EngineCore/Threading/JobSystem/Job.h>
#include <EngineCore/Threading/JobSystem/WorkerThread.h>

#include <Utils/PrimitiveDefs.h>

namespace Threading
{
	class JobScheduler
	{
		friend class WorkerThread;

	public:
		JobScheduler();
		~JobScheduler();

	public:
		static JobScheduler& Get()
		{
			static JobScheduler jobScheduler;
			return jobScheduler;
		}

	public:
		void Initialize();

		// Jobs still sitting in the queue are abandoned (never run), in-flight jobs are allowed to finish,
		// then all worker threads are joined.
		void ShutDown();

	public:
		// Safe to call from any thread. The returned handle can be ignored for fire-and-forget work.
		JobHandle PushJob(Job::Task task);

		// Blocks the calling thread until the job has completed. Do not call from a worker thread
		// on a job that may be queued behind the caller.
		void Wait(const JobHandle& handle);

		// Blocks the calling thread until the queue is empty and no job is executing.
		void WaitForIdle();

		size_t GetQueueSize() const;

	private:
		// Called by worker threads. Blocks until a job is available; returns false when the worker should exit.
		bool WaitForJob(Job& outJob);
		void OnJobFinished();

	private:
		mutable std::mutex m_mutex;
		std::condition_variable m_jobAvailableCV;
		std::condition_variable m_jobCompletedCV;

		std::queue<Job> m_jobQueue;
		U32 m_activeJobCount = 0;

		bool m_isInitialized = false;
		bool m_isShuttingDown = false;

		WorkerThread m_workerThreads[MAX_WORKER_THREADS];
	};
}
