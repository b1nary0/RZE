#include <StdAfx.h>
#include <EngineCore/Threading/JobSystem/Job.h>

namespace Threading
{
	Job::Job()
	{
	}

	Job::Job(Task task)
		: mTask(task)
	{
	}

	Job::Job(Job&& other)
		: mTask(std::move(other.mTask))
	{
	}

	Job::Job(const Job& other)
		: mTask(other.mTask)
	{
	}

	Job::~Job()
	{
	}

	void Job::Run()
	{
		if (mTask != nullptr)
			mTask.Call();
	}

	Job::Task Job::GetTask() const
	{
		return mTask;
	}
}
