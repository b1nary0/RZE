#include <StdAfx.h>
#include <EngineCore/Threading/JobSystem/JobScheduler.h>

#include <Utils/PrimitiveDefs.h>
#include <Utils/DebugUtils/Debug.h>

namespace Threading
{
	std::mutex JobScheduler::JobMutex;

	JobScheduler::JobScheduler()
		: bShouldWait(false)
	{

	}

	JobScheduler::~JobScheduler()
	{
		for (int i = 0; i < MAX_WORKER_THREADS; ++i)
		{
			AssertExpr(!mWorkerThreads[i].IsRunning());
		}
	}

	void JobScheduler::Initialize()
	{
		for (int i = 0; i < MAX_WORKER_THREADS; ++i)
		{
			mWorkerThreads[i].Initialize();
		}
	}

	void JobScheduler::ShutDown()
	{
		for (int i = 0; i < MAX_WORKER_THREADS; ++i)
		{
			mWorkerThreads[i].ShutDown();
		}
	}

	void JobScheduler::PushJob(Job::Task task)
	{
		std::unique_lock<std::mutex> lock(JobMutex);
		mJobQueue.emplace(task);
	}

	bool JobScheduler::RequestJob(Job& outJob)
	{
		// #TODO(Have each thread have a queue and send it to whoever is free at the moment
		//       instead of one queue because we're losing time to a locked mutex)
		std::unique_lock<std::mutex> lock(JobMutex, std::defer_lock);
		if (!mJobQueue.empty() && lock.try_lock())
		{
			Job job = mJobQueue.front();
			outJob = std::move(job);
			mJobQueue.pop();
			return true;
		}

		return false;
	}

	void JobScheduler::Wait()
	{
		bShouldWait = true;
		while (bShouldWait)
		{
			if (mJobQueue.empty())
			{
				U32 idleThreads = 0;
				for (U32 workerIndex = 0; workerIndex < MAX_WORKER_THREADS; ++workerIndex)
				{
					if (mWorkerThreads[workerIndex].IsIdle())
					{
						++idleThreads;
					}
				}

				if (idleThreads == MAX_WORKER_THREADS)
				{
					bShouldWait = false;
				}
			}
		}
	}
}