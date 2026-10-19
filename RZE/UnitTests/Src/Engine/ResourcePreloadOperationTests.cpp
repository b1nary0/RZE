#include <doctest/doctest.h>

#include <RZE.h>

#include <EngineCore/Async/AsyncOperationManager.h>
#include <EngineCore/Resources/ResourceDependencyList.h>
#include <EngineCore/Resources/ResourcePreloadOperation.h>

#include <Utils/Interfaces/Resource.h>
#include <Utils/Platform/Filepath.h>

#include <memory>
#include <string>

// A preload loads through the engine's ResourceHandler. These use resources that don't load asynchronously, which it
// loads on the spot through the handler, so no worker threads or renderer are needed.

namespace
{
	// Loads without touching a file, unless its path names it as missing
	class FakeResource : public IResource
	{
	public:
		bool Load(const Filepath& filePath) override { return filePath.GetRelativePath().find("Missing") == std::string::npos; }
		void Release() override {}
	};

	const Filepath k_preloadedPath("Assets/UnitTests/Preloaded.resource");
	const Filepath k_missingPath("Assets/UnitTests/Missing.resource");
	const Filepath k_otherPath("Assets/UnitTests/Other.resource");

	// Ticks until operation has finished, or gives up
	void TickUntilFinished(AsyncOperationManager& manager, const AsyncOperation& operation)
	{
		for (int tick = 0; tick < 10 && !operation.IsFinished(); ++tick)
		{
			manager.Tick();
		}
	}
}

TEST_CASE("A preload hands out the resources it loaded, and only those, and its handles keep them loaded")
{
	ResourceHandler& handler = RZE().GetResourceHandler();
	AsyncOperationManager manager;
	manager.Initialize();

	ResourceDependencyList resources;
	resources.Add<FakeResource>(k_preloadedPath.GetRelativePath());
	resources.Add<FakeResource>(k_missingPath.GetRelativePath());
	std::shared_ptr<ResourcePreloadOperation> preload = std::make_shared<ResourcePreloadOperation>(std::move(resources), EAsyncOperationFlags::None);

	// Nothing's loaded before it starts
	CHECK_FALSE(preload->GetHandle(k_preloadedPath).IsValid());

	manager.Start(preload);
	TickUntilFinished(manager, *preload);
	REQUIRE(preload->Succeeded());

	ResourceHandle preloaded = preload->GetHandle(k_preloadedPath);
	CHECK(preloaded.IsValid());
	// One that failed, and one that's loaded but not by this preload
	CHECK_FALSE(preload->GetHandle(k_missingPath).IsValid());
	ResourceHandle other = handler.LoadResource<FakeResource>(k_otherPath);
	REQUIRE(other.IsValid());
	CHECK_FALSE(preload->GetHandle(k_otherPath).IsValid());

	// Once the preload's gone, the handle it gave out is what keeps the resource loaded
	preload.reset();
	manager.ShutDown();
	CHECK(handler.HasResource(k_preloadedPath));
	handler.ReleaseResource(preloaded);
	CHECK_FALSE(handler.HasResource(k_preloadedPath));

	handler.ReleaseResource(other);
}

TEST_CASE("A preload goes by the name it's given, and says what it's doing from the start")
{
	ResourceHandler& handler = RZE().GetResourceHandler();
	AsyncOperationManager manager;
	manager.Initialize();

	ResourceDependencyList resources;
	resources.Add<FakeResource>(k_preloadedPath.GetRelativePath());
	std::shared_ptr<ResourcePreloadOperation> preload = std::make_shared<ResourcePreloadOperation>(std::move(resources), EAsyncOperationFlags::None, "Loading Preloaded.resource");
	CHECK(preload->GetName() == "Loading Preloaded.resource");
	CHECK(preload->GetKind() == EAsyncOperationKind::ResourceLoad);

	manager.Start(preload);
	manager.Tick();
	CHECK_FALSE(preload->GetStatusText().empty());

	TickUntilFinished(manager, *preload);
	REQUIRE(preload->Succeeded());
	CHECK(preload->GetProgress() == doctest::Approx(1.0f));

	ResourceHandle preloaded = preload->GetHandle(k_preloadedPath);
	preload.reset();
	manager.ShutDown();
	handler.ReleaseResource(preloaded);
}
