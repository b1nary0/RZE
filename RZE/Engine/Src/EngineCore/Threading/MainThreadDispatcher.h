#pragma once

#include <vector>

#include <EngineCore/Threading/Threading.h>
#include <EngineCore/Threading/JobSystem/Job.h>

namespace Threading
{
	// Lets any thread schedule work to run on the main thread. Posted tasks are executed in
	// submission order at the start of the next frame (see RZE_Engine::PreUpdate).
	class MainThreadDispatcher
	{
	public:
		static MainThreadDispatcher& Get()
		{
			static MainThreadDispatcher dispatcher;
			return dispatcher;
		}

	public:
		// Must be called from the main thread; records its thread id.
		void Initialize();

		// Discards anything still pending and rejects further posts.
		void ShutDown();

	public:
		// Safe to call from any thread.
		void Post(Job::Task task);

		// Main thread only. Runs every task that was posted before this call. Tasks posted while
		// draining (including by the tasks themselves) run on the next Drain().
		void Drain();

		bool IsMainThread() const;

	private:
		MainThreadDispatcher() = default;

	private:
		mutable std::mutex m_mutex;
		std::vector<Job::Task> m_pendingTasks;
		std::thread::id m_mainThreadID;
		bool m_isShutDown = false;
	};
}
