#include <doctest/doctest.h>

#include <EngineCore/Threading/JobSystem/JobScheduler.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <functional>
#include <thread>

namespace
{
	// Polls until condition holds or a few seconds pass, so a broken scheduler fails the test rather than hanging it
	bool WaitUntil(const std::function<bool()>& condition)
	{
		const auto giveUpAt = std::chrono::steady_clock::now() + std::chrono::seconds(5);
		while (!condition())
		{
			if (std::chrono::steady_clock::now() > giveUpAt)
			{
				return false;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
		return true;
	}
}

TEST_CASE("Background jobs never take every worker, so a foreground job still runs while they're busy")
{
	Threading::JobScheduler& scheduler = Threading::JobScheduler::Get();
	scheduler.Initialize();

	std::atomic<bool> releaseBackground{ false };
	std::atomic<U32> runningBackground{ 0 };
	std::atomic<U32> mostRunningBackground{ 0 };

	// More than the lane can run at once
	const U32 backgroundJobCount = Threading::JobScheduler::k_maxActiveBackgroundJobs + 2;
	for (U32 jobIndex = 0; jobIndex < backgroundJobCount; ++jobIndex)
	{
		scheduler.PushJob(Threading::Job::Task([&]()
			{
				const U32 running = ++runningBackground;
				U32 most = mostRunningBackground.load();
				while (running > most && !mostRunningBackground.compare_exchange_weak(most, running))
				{
				}

				while (!releaseBackground.load())
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(1));
				}
				--runningBackground;
			}), Threading::EJobLane::Background);
	}

	CHECK(WaitUntil([&]() { return runningBackground.load() == Threading::JobScheduler::k_maxActiveBackgroundJobs; }));
	// Give any worker that shouldn't take one the chance to
	std::this_thread::sleep_for(std::chrono::milliseconds(50));
	CHECK(mostRunningBackground.load() == Threading::JobScheduler::k_maxActiveBackgroundJobs);

	std::atomic<bool> foregroundRan{ false };
	const Threading::JobHandle foreground = scheduler.PushJob(Threading::Job::Task([&]() { foregroundRan = true; }));
	CHECK(WaitUntil([&]() { return foreground.IsComplete(); }));
	CHECK(foregroundRan.load());

	releaseBackground = true;
	scheduler.WaitForIdle();
	CHECK(runningBackground.load() == 0);
	CHECK(mostRunningBackground.load() == Threading::JobScheduler::k_maxActiveBackgroundJobs);

	scheduler.ShutDown();
}
