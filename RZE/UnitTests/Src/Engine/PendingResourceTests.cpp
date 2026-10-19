#include <doctest/doctest.h>

#include <RZE.h>

#include <EngineCore/Async/AsyncOperationManager.h>
#include <EngineCore/Resources/PendingResource.h>

#include <Utils/Interfaces/Resource.h>
#include <Utils/Platform/Filepath.h>

#include <memory>
#include <string>

// A PendingResource loads through the engine's ResourceHandler and AsyncOperationManager. These use resources that don't
// load asynchronously, which the handler loads on the spot as the load starts, so no worker threads or renderer are
// needed.

namespace
{
	// Loads without touching a file, unless its path names it as missing
	class FakeResource : public IResource
	{
	public:
		bool Load(const Filepath& filePath) override { return filePath.GetRelativePath().find("Missing") == std::string::npos; }
		void Release() override {}
	};

	const Filepath k_loadedPath("Assets/UnitTests/PendingLoaded.resource");
	const Filepath k_otherPath("Assets/UnitTests/PendingOther.resource");
	const Filepath k_missingPath("Assets/UnitTests/PendingMissing.resource");
	const Filepath k_thirdPath("Assets/UnitTests/PendingThird.resource");

	// The engine's operation manager, running for the test
	struct EngineOperations
	{
		AsyncOperationManager& Manager = RZE().GetAsyncOperationManager();

		EngineOperations() { Manager.Initialize(); }
		~EngineOperations() { Manager.ShutDown(); }

		// Ticks until pending has its resource, or gives up
		void TickUntilLoaded(PendingResource& pending)
		{
			for (int tick = 0; tick < 10 && pending.IsLoading(); ++tick)
			{
				Manager.Tick();
			}
		}

		// Long enough for finished operations to be cleaned up and let go of
		void TickUntilRetired()
		{
			for (U32 tick = 0; tick <= AsyncOperationManager::k_retireFrameCount + 1; ++tick)
			{
				Manager.Tick();
			}
		}
	};
}

TEST_CASE_FIXTURE(EngineOperations, "A pending resource takes one that's already loaded straight away")
{
	ResourceHandler& handler = RZE().GetResourceHandler();
	ResourceHandle loaded = handler.LoadResource<FakeResource>(k_loadedPath);
	REQUIRE(loaded.IsValid());

	PendingResource pending;
	CHECK(pending.Load<FakeResource>(k_loadedPath) == nullptr);
	CHECK_FALSE(pending.IsLoading());
	CHECK(pending.GetHandle().IsValid());
	CHECK_FALSE(pending.HasFailed());
	CHECK(pending.GetPath().GetRelativePath() == k_loadedPath.GetRelativePath());

	pending.Reset();
	handler.ReleaseResource(loaded);
	CHECK_FALSE(handler.HasResource(k_loadedPath));
}

TEST_CASE_FIXTURE(EngineOperations, "A pending resource loads in the background, and its handle keeps the resource loaded once the load's gone")
{
	ResourceHandler& handler = RZE().GetResourceHandler();

	PendingResource pending;
	std::shared_ptr<AsyncOperation> load = pending.Load<FakeResource>(k_loadedPath);
	REQUIRE(load != nullptr);
	CHECK(load->GetName() == "Loading PendingLoaded.resource");

	TickUntilLoaded(pending);
	CHECK_FALSE(pending.IsLoading());
	CHECK(pending.GetHandle().IsValid());
	CHECK_FALSE(pending.HasFailed());

	load.reset();
	TickUntilRetired();
	CHECK(handler.HasResource(k_loadedPath));

	pending.Reset();
	CHECK_FALSE(pending.GetHandle().IsValid());
	CHECK_FALSE(handler.HasResource(k_loadedPath));
}

TEST_CASE_FIXTURE(EngineOperations, "A pending resource that can't be loaded has failed")
{
	PendingResource pending;
	REQUIRE(pending.Load<FakeResource>(k_missingPath) != nullptr);

	TickUntilLoaded(pending);
	CHECK_FALSE(pending.IsLoading());
	CHECK(pending.HasFailed());
	CHECK_FALSE(pending.GetHandle().IsValid());
}

TEST_CASE_FIXTURE(EngineOperations, "Resetting a pending resource while it loads cancels the load")
{
	PendingResource pending;
	std::shared_ptr<AsyncOperation> load = pending.Load<FakeResource>(k_loadedPath);
	REQUIRE(load != nullptr);

	pending.Reset();
	CHECK(load->IsCancellationRequested());
	CHECK_FALSE(pending.IsLoading());
	CHECK_FALSE(pending.GetPath().IsValid());

	Manager.Tick();
	CHECK(load->GetState() == EAsyncOperationState::Cancelled);
}

TEST_CASE_FIXTURE(EngineOperations, "Loading again, or moving another pending resource in, cancels the load it replaces")
{
	PendingResource pending;
	std::shared_ptr<AsyncOperation> first = pending.Load<FakeResource>(k_loadedPath);
	std::shared_ptr<AsyncOperation> second = pending.Load<FakeResource>(k_otherPath);
	REQUIRE(first != nullptr);
	REQUIRE(second != nullptr);
	CHECK(first->IsCancellationRequested());
	CHECK_FALSE(second->IsCancellationRequested());
	CHECK(pending.GetPath().GetRelativePath() == k_otherPath.GetRelativePath());

	PendingResource other;
	std::shared_ptr<AsyncOperation> third = other.Load<FakeResource>(k_thirdPath);
	REQUIRE(third != nullptr);
	pending = std::move(other);
	CHECK(second->IsCancellationRequested());
	CHECK_FALSE(third->IsCancellationRequested());
	CHECK(pending.GetPath().GetRelativePath() == k_thirdPath.GetRelativePath());

	TickUntilLoaded(pending);
	CHECK(pending.GetHandle().IsValid());
}
