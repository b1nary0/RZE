#pragma once

#include <EngineCore/Threading/Threading.h>

#include <Utils/Functor.h>

namespace Threading
{
	// Shared between a Job and every JobHandle that refers to it, so a handle
	// stays valid regardless of whether the job (or the scheduler) is still alive.
	struct JobState
	{
		std::atomic<bool> m_isComplete{ false };
		std::atomic<bool> m_wasAbandoned{ false };
	};

	class JobHandle
	{
		friend class JobScheduler;

	public:
		JobHandle() = default;

	public:
		bool IsValid() const;

		// True once the job has finished running (or was abandoned). An invalid handle is considered complete.
		bool IsComplete() const;

		// True if the job never ran because the scheduler shut down before it was picked up.
		bool WasAbandoned() const;

	private:
		explicit JobHandle(std::shared_ptr<JobState> state);

	private:
		std::shared_ptr<JobState> m_state;
	};

	class Job
	{
	public:
		// NOTE: Backed by std::function, so anything captured by a task must be copyable.
		using Task = Functor<void>;

	public:
		Job() = default;
		Job(Task task, std::shared_ptr<JobState> state);

	public:
		void Run();

		// Marks the job complete without running it. Used when the scheduler shuts down with jobs still queued.
		void Abandon();

	private:
		void MarkComplete();

	private:
		Task m_task;
		std::shared_ptr<JobState> m_state;
	};
}
