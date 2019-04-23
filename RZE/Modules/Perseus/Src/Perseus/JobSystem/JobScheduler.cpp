#include <Perseus/JobSystem/JobScheduler.h>

#include <Utils/DebugUtils/Debug.h>

namespace Perseus
{
	std::mutex JobScheduler::JobMutex;

	JobScheduler::JobScheduler()
		: bShouldWait(false)
	{

	}

	JobScheduler::~JobScheduler()
	{
		for (int i = 0; i < PERSEUS_MAX_WORKER_THREADS; ++i)
		{
			AssertExpr(!mWorkerThreads[i].IsRunning());
		}
	}

	void JobScheduler::Initialize()
	{
		int conc = std::thread::hardware_concurrency();
		for (int i = 0; i < PERSEUS_MAX_WORKER_THREADS; ++i)
		{
			mWorkerThreads[i].Initialize();
		}
	}

	void JobScheduler::ShutDown()
	{
		for (int i = 0; i < PERSEUS_MAX_WORKER_THREADS; ++i)
		{
			mWorkerThreads[i].Finish();
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
		if (lock.try_lock() && !mJobQueue.empty())
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
				bShouldWait = false;
			}
		}
	}
}