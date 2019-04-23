#include <Perseus/JobSystem/WorkerThread.h>

#include <Perseus/JobSystem/JobScheduler.h>

#include <Utils/Functor.h>

#include <Optick/optick.h>

namespace Perseus
{
	WorkerThread::WorkerThread()
		: bActive(false)
		, bIdle(true)
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
		bActive = false;
		mThread.join();
		bRunning = false;
	}

	bool WorkerThread::IsIdle()
	{
		return bIdle;
	}

	void WorkerThread::ThreadSetup()
	{
		auto exec([this]()
		{
			OPTICK_THREAD("Worker Thread");
			while (bActive)
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

		bActive = true;
		mThread = std::thread(exec);
		bRunning = true;
	}

	bool WorkerThread::IsRunning()
	{
		return bRunning;
	}

}