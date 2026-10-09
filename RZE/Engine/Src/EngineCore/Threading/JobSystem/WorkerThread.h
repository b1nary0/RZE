#pragma once

#include <EngineCore/Threading/Threading.h>

#include <Utils/PrimitiveDefs.h>

namespace Threading
{
	class JobScheduler;

	class WorkerThread
	{
	public:
		WorkerThread() = default;
		~WorkerThread() = default;

		WorkerThread(const WorkerThread&) = delete;
		WorkerThread& operator=(const WorkerThread&) = delete;

	public:
		void Start(JobScheduler& scheduler, U32 workerIndex);
		void Join();

		bool IsRunning() const;

	private:
		void ThreadMain(JobScheduler& scheduler);

	private:
		std::thread m_thread;
		U32 m_workerIndex = 0;
	};
}
