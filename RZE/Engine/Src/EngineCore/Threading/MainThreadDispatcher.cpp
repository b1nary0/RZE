#include <StdAfx.h>
#include <EngineCore/Threading/MainThreadDispatcher.h>

#include <Utils/DebugUtils/Debug.h>

namespace Threading
{
	void MainThreadDispatcher::Initialize()
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_mainThreadID = std::this_thread::get_id();
		m_isShutDown = false;
	}

	void MainThreadDispatcher::ShutDown()
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_isShutDown = true;
		m_pendingTasks.clear();
	}

	void MainThreadDispatcher::Post(Job::Task task)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		if (m_isShutDown)
		{
			return;
		}

		m_pendingTasks.emplace_back(std::move(task));
	}

	void MainThreadDispatcher::Drain()
	{
		OPTICK_EVENT();
		AssertExpr(IsMainThread());

		std::vector<Job::Task> tasksToRun;
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			tasksToRun.swap(m_pendingTasks);
		}

		// Run outside the lock so tasks are free to Post() more work.
		for (Job::Task& task : tasksToRun)
		{
			task.Call();
		}
	}

	bool MainThreadDispatcher::IsMainThread() const
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		return std::this_thread::get_id() == m_mainThreadID;
	}
}
