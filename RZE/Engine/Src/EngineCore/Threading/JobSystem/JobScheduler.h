#pragma once

#include <queue>

#include <EngineCore/Threading/Threading.h>
#include <EngineCore/Threading/JobSystem/Job.h>
#include <EngineCore/Threading/JobSystem/WorkerThread.h>

#include <Utils/PrimitiveDefs.h>

namespace Threading
{
	enum class EJobLane : U8
	{
		// Work someone is waiting on, e.g. loading the scene being opened
		Foreground,
		// Work nobody is waiting on, e.g. rendering thumbnails. A worker only takes it when no foreground job is
		// queued, and it never runs on every worker at once, so a foreground job always has a worker to start on.
		Background
	};

	class JobScheduler
	{
		friend class WorkerThread;

	public:
		// Background jobs run on at most this many workers at once
		static constexpr U32 k_maxActiveBackgroundJobs = MAX_WORKER_THREADS - 1;
		static_assert(k_maxActiveBackgroundJobs > 0, "Background jobs need a worker of their own");

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
		JobHandle PushJob(Job::Task task, EJobLane lane = EJobLane::Foreground);

		// Blocks the calling thread until the job has completed. Do not call from a worker thread
		// on a job that may be queued behind the caller.
		void Wait(const JobHandle& handle);

		// Blocks the calling thread until both queues are empty and no job is executing.
		void WaitForIdle();

		// Jobs waiting in both lanes
		size_t GetQueueSize() const;

	private:
		// Called by worker threads. Blocks until a job is available; returns false when the worker should exit.
		bool WaitForJob(Job& outJob, EJobLane& outLane);
		void OnJobFinished(EJobLane lane);

		// m_mutex must be held
		bool CanStartBackgroundJob() const;

	private:
		mutable std::mutex m_mutex;
		std::condition_variable m_jobAvailableCV;
		std::condition_variable m_jobCompletedCV;

		std::queue<Job> m_jobQueue;
		std::queue<Job> m_backgroundJobQueue;
		U32 m_activeJobCount = 0;
		U32 m_activeBackgroundJobCount = 0;

		bool m_isInitialized = false;
		bool m_isShuttingDown = false;

		WorkerThread m_workerThreads[MAX_WORKER_THREADS];
	};
}
