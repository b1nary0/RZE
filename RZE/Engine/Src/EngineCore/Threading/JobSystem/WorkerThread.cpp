#include <StdAfx.h>
#include <EngineCore/Threading/JobSystem/WorkerThread.h>

#include <EngineCore/Threading/JobSystem/JobScheduler.h>

#include <Optick/optick.h>

namespace Threading
{
	void WorkerThread::Start(JobScheduler& scheduler, U32 workerIndex)
	{
		AssertExpr(!m_thread.joinable());

		m_workerIndex = workerIndex;
		m_thread = std::thread([this, &scheduler]() { ThreadMain(scheduler); });
	}

	void WorkerThread::Join()
	{
		if (m_thread.joinable())
		{
			m_thread.join();
		}
	}

	bool WorkerThread::IsRunning() const
	{
		return m_thread.joinable();
	}

	void WorkerThread::ThreadMain(JobScheduler& scheduler)
	{
		OPTICK_THREAD("Worker Thread");

		Job job;
		while (scheduler.WaitForJob(job))
		{
			job.Run();
			job = Job();

			scheduler.OnJobFinished();
		}
	}
}
