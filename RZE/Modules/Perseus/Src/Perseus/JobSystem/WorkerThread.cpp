#include <Perseus/JobSystem/WorkerThread.h>

#include <Perseus/JobSystem/JobScheduler.h>

#include <Utils/Functor.h>

namespace Perseus
{
	WorkerThread::WorkerThread()
		: bIdle(true)
		, bRunning(false)
	{
		static int count = 1;
		mThreadID = count;
		++count;
	}

	WorkerThread::~WorkerThread()
	{
	}

	void WorkerThread::Initialize()
	{
		ThreadSetup();
	}

	void WorkerThread::Finish()
	{
		bRunning = false;
		mThread.join();
	}

	void WorkerThread::ThreadSetup()
	{
		auto exec([this]()
		{
			while (bRunning)
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

		bRunning = true;
		mThread = std::thread(exec);
	}
}