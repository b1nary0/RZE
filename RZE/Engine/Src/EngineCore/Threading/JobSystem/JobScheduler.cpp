#include <StdAfx.h>
#include <EngineCore/Threading/JobSystem/JobScheduler.h>

#include <Utils/DebugUtils/Debug.h>

namespace Threading
{
	JobScheduler::JobScheduler()
	{
	}

	JobScheduler::~JobScheduler()
	{
		for (int i = 0; i < MAX_WORKER_THREADS; ++i)
		{
			AssertExpr(!m_workerThreads[i].IsRunning());
		}
	}

	void JobScheduler::Initialize()
	{
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			if (m_isInitialized)
			{
				return;
			}

			m_isInitialized = true;
			m_isShuttingDown = false;
		}

		for (U32 i = 0; i < MAX_WORKER_THREADS; ++i)
		{
			m_workerThreads[i].Start(*this, i);
		}
	}

	void JobScheduler::ShutDown()
	{
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			if (!m_isInitialized)
			{
				return;
			}

			m_isShuttingDown = true;

			for (std::queue<Job>* queue : { &m_jobQueue, &m_backgroundJobQueue })
			{
				while (!queue->empty())
				{
					queue->front().Abandon();
					queue->pop();
				}
			}
		}

		m_jobAvailableCV.notify_all();

		for (int i = 0; i < MAX_WORKER_THREADS; ++i)
		{
			m_workerThreads[i].Join();
		}

		{
			std::lock_guard<std::mutex> lock(m_mutex);
			m_isInitialized = false;
		}

		m_jobCompletedCV.notify_all();
	}

	JobHandle JobScheduler::PushJob(Job::Task task, EJobLane lane)
	{
		std::shared_ptr<JobState> state = std::make_shared<JobState>();
		Job job(std::move(task), state);

		{
			std::lock_guard<std::mutex> lock(m_mutex);
			if (m_isShuttingDown || !m_isInitialized)
			{
				RZE_LOG("JobScheduler::PushJob called while the scheduler is not running. Job abandoned.");
				job.Abandon();
				return JobHandle(state);
			}

			(lane == EJobLane::Background ? m_backgroundJobQueue : m_jobQueue).emplace(std::move(job));
		}

		m_jobAvailableCV.notify_one();
		return JobHandle(state);
	}

	void JobScheduler::Wait(const JobHandle& handle)
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		m_jobCompletedCV.wait(lock, [&handle]() { return handle.IsComplete(); });
	}

	void JobScheduler::WaitForIdle()
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		m_jobCompletedCV.wait(lock, [this]() { return m_jobQueue.empty() && m_backgroundJobQueue.empty() && m_activeJobCount == 0; });
	}

	size_t JobScheduler::GetQueueSize() const
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		return m_jobQueue.size() + m_backgroundJobQueue.size();
	}

	bool JobScheduler::WaitForJob(Job& outJob, EJobLane& outLane)
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		m_jobAvailableCV.wait(lock, [this]() { return m_isShuttingDown || !m_jobQueue.empty() || CanStartBackgroundJob(); });

		if (m_isShuttingDown)
		{
			return false;
		}

		// Foreground first
		if (!m_jobQueue.empty())
		{
			outJob = std::move(m_jobQueue.front());
			m_jobQueue.pop();
			outLane = EJobLane::Foreground;
		}
		else
		{
			outJob = std::move(m_backgroundJobQueue.front());
			m_backgroundJobQueue.pop();
			outLane = EJobLane::Background;
			++m_activeBackgroundJobCount;
		}
		++m_activeJobCount;

		return true;
	}

	void JobScheduler::OnJobFinished(EJobLane lane)
	{
		bool hasFreedBackgroundWorker = false;
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			AssertExpr(m_activeJobCount > 0);
			--m_activeJobCount;

			if (lane == EJobLane::Background)
			{
				AssertExpr(m_activeBackgroundJobCount > 0);
				--m_activeBackgroundJobCount;
				hasFreedBackgroundWorker = !m_backgroundJobQueue.empty();
			}
		}

		m_jobCompletedCV.notify_all();
		// A worker that woke for a queued background job while the lane was full went back to sleep
		if (hasFreedBackgroundWorker)
		{
			m_jobAvailableCV.notify_one();
		}
	}

	bool JobScheduler::CanStartBackgroundJob() const
	{
		return !m_backgroundJobQueue.empty() && m_activeBackgroundJobCount < k_maxActiveBackgroundJobs;
	}
}
