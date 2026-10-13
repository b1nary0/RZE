#include <doctest/doctest.h>

#include <EngineCore/Resources/ResourceHandler.h>

#include <Utils/Interfaces/Resource.h>
#include <Utils/Platform/Filepath.h>

namespace
{
	struct ResourceCounts
	{
		int Released = 0;
		int Deleted = 0;
	};

	// Loads without touching a file, and counts its release and deletion
	class CountedResource : public IResource
	{
	public:
		explicit CountedResource(ResourceCounts* counts)
			: m_counts(counts) {}
		~CountedResource() override { ++m_counts->Deleted; }

		bool Load(const Filepath& filePath) override { return true; }
		void Release() override { ++m_counts->Released; }

	private:
		ResourceCounts* m_counts;
	};

	const Filepath k_path("Assets/UnitTests/Counted.resource");
}

// No renderer runs in the tests, so no frame is ever processed: whatever is waiting on the render thread is only
// destroyed by ShutDown.

TEST_CASE("A released resource is kept until the render thread is done with it, but can be loaded again straight away")
{
	ResourceHandler handler;
	ResourceCounts counts;

	{
		ResourceHandle handle = handler.LoadResource<CountedResource>(k_path, &counts);
		REQUIRE(handle.IsValid());
	}

	CHECK_FALSE(handler.HasResource(k_path));
	handler.Update();
	CHECK(counts.Released == 0);
	CHECK(counts.Deleted == 0);

	ResourceCounts reloadCounts;
	{
		ResourceHandle reloaded = handler.LoadResource<CountedResource>(k_path, &reloadCounts);
		CHECK(reloaded.IsValid());
	}

	handler.ShutDown();
	CHECK(counts.Released == 1);
	CHECK(counts.Deleted == 1);
	CHECK(reloadCounts.Deleted == 1);
}

TEST_CASE("A duplicate registration keeps the first resource and destroys the second only once it's rendered")
{
	ResourceHandler handler;
	ResourceCounts firstCounts;
	ResourceCounts duplicateCounts;

	{
		ResourceHandle first = handler.RegisterLoadedResource(k_path, new CountedResource(&firstCounts));
		ResourceHandle duplicate = handler.RegisterLoadedResource(k_path, new CountedResource(&duplicateCounts));

		CHECK(duplicate.IsValid());
		CHECK(handler.GetResource<CountedResource>(duplicate) == handler.GetResource<CountedResource>(first));

		handler.Update();
		CHECK(duplicateCounts.Deleted == 0);
		CHECK(firstCounts.Deleted == 0);
	}

	handler.ShutDown();
	CHECK(duplicateCounts.Released == 1);
	CHECK(duplicateCounts.Deleted == 1);
	CHECK(firstCounts.Deleted == 1);
}

TEST_CASE("Only one loader at a time can load a resource asynchronously")
{
	ResourceHandler handler;
	const std::string key = ResourceHandler::CreateResourceKey(k_path);
	const int firstLoader = 0;
	const int secondLoader = 0;

	CHECK_FALSE(handler.IsAsyncLoadInProgress(key));
	CHECK(handler.TryBeginAsyncLoad(key, &firstLoader));
	CHECK(handler.IsAsyncLoadInProgress(key));

	CHECK_FALSE(handler.TryBeginAsyncLoad(key, &secondLoader));

	handler.EndAsyncLoad(key, &firstLoader);
	CHECK_FALSE(handler.IsAsyncLoadInProgress(key));
	CHECK(handler.TryBeginAsyncLoad(key, &secondLoader));

	handler.EndAsyncLoad(key, &secondLoader);
	handler.ShutDown();
}
