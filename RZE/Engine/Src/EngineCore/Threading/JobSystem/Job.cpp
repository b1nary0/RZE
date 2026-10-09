#include <StdAfx.h>
#include <EngineCore/Threading/JobSystem/Job.h>

namespace Threading
{
	JobHandle::JobHandle(std::shared_ptr<JobState> state)
		: m_state(std::move(state))
	{
	}

	bool JobHandle::IsValid() const
	{
		return m_state != nullptr;
	}

	bool JobHandle::IsComplete() const
	{
		return m_state == nullptr || m_state->m_isComplete.load(std::memory_order_acquire);
	}

	bool JobHandle::WasAbandoned() const
	{
		return m_state != nullptr && m_state->m_wasAbandoned.load(std::memory_order_acquire);
	}

	Job::Job(Task task, std::shared_ptr<JobState> state)
		: m_task(std::move(task))
		, m_state(std::move(state))
	{
	}

	void Job::Run()
	{
		if (m_task != nullptr)
		{
			try
			{
				m_task.Call();
			}
			catch (const std::exception& e)
			{
				RZE_LOG_ARGS("Job threw an exception: %s", e.what());
			}
			catch (...)
			{
				RZE_LOG("Job threw an unknown exception.");
			}
		}

		MarkComplete();
	}

	void Job::Abandon()
	{
		if (m_state != nullptr)
		{
			m_state->m_wasAbandoned.store(true, std::memory_order_release);
		}

		MarkComplete();
	}

	void Job::MarkComplete()
	{
		if (m_state != nullptr)
		{
			// Release pairs with the acquire in JobHandle::IsComplete() so any data the task wrote
			// is visible to whoever observes completion.
			m_state->m_isComplete.store(true, std::memory_order_release);
		}
	}
}
