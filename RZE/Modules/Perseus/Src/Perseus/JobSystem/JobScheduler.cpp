#include <Perseus/JobSystem/JobScheduler.h>

#include <Utils/DebugUtils/Debug.h>

namespace Perseus
{
	std::mutex JobScheduler::JobMutex;

	JobScheduler::JobScheduler()
	{
		for (int i = 0; i < PERSEUS_MAX_WORKER_THREADS; ++i)
		{
			mWorkerThreads[i].Initialize();
		}
	}

	void JobScheduler::PushJob(Job::Task task)
	{
		std::unique_lock<std::mutex> lock(JobMutex);
		mJobQueue.emplace(task);
	}

	bool JobScheduler::RequestJob(Job& outJob)
	{
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
		while (true)
		{
			int count = 0;
			for (int i = 0; i < PERSEUS_MAX_WORKER_THREADS; ++i)
			{
				if (mWorkerThreads[i].bIdle)
				{
					++count;
				}
			}

			if (count == PERSEUS_MAX_WORKER_THREADS)
			{
				break;
			}
		}
	}
}