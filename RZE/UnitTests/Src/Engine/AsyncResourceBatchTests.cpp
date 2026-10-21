#include <doctest/doctest.h>

#include <RZE.h>

#include <EngineCore/Async/AsyncOperationManager.h>
#include <EngineCore/Resources/ResourceDependencyList.h>
#include <EngineCore/Resources/ResourcePreloadOperation.h>
#include <EngineCore/Threading/JobSystem/JobScheduler.h>

#include <Utils/Interfaces/Resource.h>
#include <Utils/Platform/Filepath.h>

#include <chrono>
#include <memory>
#include <string>
#include <thread>

// A batch is driven here by a preload, which loads through the engine's ResourceHandler. Its CPU loads run on the job
// scheduler's workers; nothing needs a renderer.

namespace
{
	struct AsyncResourceCounts
	{
		int FinalizeSteps = 0;
		int Released = 0;
		int Deleted = 0;
	};

	// A preload makes its resources itself, so they count into this
	AsyncResourceCounts g_counts;

	// Loads asynchronously without touching a file. Its CPU load fails if its path names it as missing, and it never
	// finishes finalizing if its path names it as stuck.
	class AsyncCountedResource : public IResource
	{
	public:
		~AsyncCountedResource() override { ++g_counts.Deleted; }

		bool Load(const Filepath& filePath) override { return false; }
		void Release() override { ++g_counts.Released; }

		bool SupportsAsyncLoad() const override { return true; }
		bool LoadCPU(const Filepath& filePath) override
		{
			m_isStuck = filePath.GetRelativePath().find("Stuck") != std::string::npos;
			return filePath.GetRelativePath().find("Missing") == std::string::npos;
		}
		bool FinalizeStep() override
		{
			++g_counts.FinalizeSteps;
			return !m_isStuck;
		}

	private:
		bool m_isStuck = false;
	};

	const Filepath k_missingPath("Assets/UnitTests/AsyncMissing.resource");
	const Filepath k_stuckPath("Assets/UnitTests/AsyncStuck.resource");

	std::shared_ptr<ResourcePreloadOperation> MakePreload(const Filepath& resourcePath)
	{
		ResourceDependencyList resources;
		resources.Add<AsyncCountedResource>(resourcePath.GetRelativePath());
		return std::make_shared<ResourcePreloadOperation>(std::move(resources), EAsyncOperationFlags::None);
	}

	// Ticks until done says so, or gives up. The CPU loads run on workers, so this waits for them between ticks.
	template <typename TDone>
	void TickUntil(AsyncOperationManager& manager, TDone done)
	{
		for (int tick = 0; tick < 1000 && !done(); ++tick)
		{
			manager.Tick();
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}
}

TEST_CASE("A resource whose async load fails is destroyed straight away")
{
	Threading::JobScheduler::Get().Initialize();
	AsyncOperationManager manager;
	manager.Initialize();
	g_counts = AsyncResourceCounts();

	std::shared_ptr<ResourcePreloadOperation> preload = MakePreload(k_missingPath);
	manager.Start(preload);
	TickUntil(manager, [&preload]() { return preload->IsFinished(); });
	REQUIRE(preload->IsFinished());

	// While the preload that tried to load it is still around
	CHECK(g_counts.Released == 1);
	CHECK(g_counts.Deleted == 1);
	CHECK_FALSE(RZE().GetResourceHandler().HasResource(k_missingPath));

	preload.reset();
	manager.ShutDown();
	Threading::JobScheduler::Get().ShutDown();
}

TEST_CASE("A resource that's loaded but never registered is destroyed when its cancelled load is cleaned up")
{
	Threading::JobScheduler::Get().Initialize();
	AsyncOperationManager manager;
	manager.Initialize();
	g_counts = AsyncResourceCounts();

	std::shared_ptr<ResourcePreloadOperation> preload = MakePreload(k_stuckPath);
	manager.Start(preload);
	// Loaded on the CPU and finalizing, but never done
	TickUntil(manager, []() { return g_counts.FinalizeSteps > 0; });
	REQUIRE(g_counts.FinalizeSteps > 0);
	CHECK(g_counts.Deleted == 0);

	// The manager cleans an operation up a few frames after it finishes (AsyncOperationManager::k_retireFrameCount)
	preload->RequestCancel();
	TickUntil(manager, []() { return g_counts.Deleted > 0; });
	CHECK(preload->IsFinished());

	CHECK(g_counts.Released == 1);
	CHECK(g_counts.Deleted == 1);
	CHECK_FALSE(RZE().GetResourceHandler().HasResource(k_stuckPath));

	preload.reset();
	manager.ShutDown();
	Threading::JobScheduler::Get().ShutDown();
}
