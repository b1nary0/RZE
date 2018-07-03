#include <Perseus/JobSystem/Job.h>

namespace Perseus
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
