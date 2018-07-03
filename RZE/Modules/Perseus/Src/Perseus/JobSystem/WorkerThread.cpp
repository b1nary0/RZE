#include <Perseus/JobSystem/WorkerThread.h>

#include <Perseus/JobSystem/JobScheduler.h>

#include <Utils/Functor.h>

namespace Perseus
{
	WorkerThread::WorkerThread()
		: bIdle(true)
	{
		static int count = 1;
		mThreadID = count;
		++count;
	}

	WorkerThread::~WorkerThread()
	{
		mThread.join();
	}

	void WorkerThread::Initialize()
	{
		ThreadSetup();
	}

	void WorkerThread::ThreadSetup()
	{
		auto exec([this]()
		{
			while (true)
			{
				Job job;
				if (JobScheduler::Get().RequestJob(job))
				{
					bIdle = false;
					job.Run();
				}
				else
				{
					bIdle = true;
					std::this_thread::sleep_for(std::chrono::milliseconds(1));
				}
			}
		});

		mThread = std::thread(exec);
	}
}