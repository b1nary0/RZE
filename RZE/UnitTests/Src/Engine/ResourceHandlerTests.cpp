#include <doctest/doctest.h>

#include <EngineCore/Resources/ResourceHandler.h>

#include <Utils/Interfaces/Resource.h>
#include <Utils/Platform/Filepath.h>

#include <memory>
#include <string>
#include <utility>

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

	// Holds another resource, and lets go of it when it's released, like a mesh does its textures
	class HoldingResource : public IResource
	{
	public:
		HoldingResource(ResourceCounts* counts, ResourceHandle&& held)
			: m_counts(counts)
			, m_held(std::move(held)) {}
		~HoldingResource() override { ++m_counts->Deleted; }

		bool Load(const Filepath& filePath) override { return true; }
		void Release() override
		{
			++m_counts->Released;
			m_held = ResourceHandle::EmptyHandle();
		}

	private:
		ResourceCounts* m_counts;
		ResourceHandle m_held;
	};

	const Filepath k_path("Assets/UnitTests/Counted.resource");
	const Filepath k_holderPath("Assets/UnitTests/Holder.resource");
}

TEST_CASE("A released resource is destroyed straight away, and can be loaded again")
{
	ResourceHandler handler;
	ResourceCounts counts;

	{
		ResourceHandle handle = handler.LoadResource<CountedResource>(k_path, &counts);
		REQUIRE(handle.IsValid());
		CHECK(counts.Deleted == 0);
	}

	CHECK_FALSE(handler.HasResource(k_path));
	CHECK(counts.Released == 1);
	CHECK(counts.Deleted == 1);

	ResourceCounts reloadCounts;
	{
		ResourceHandle reloaded = handler.LoadResource<CountedResource>(k_path, &reloadCounts);
		CHECK(reloaded.IsValid());
	}
	CHECK(reloadCounts.Deleted == 1);

	handler.ShutDown();
	CHECK(counts.Deleted == 1);
}

TEST_CASE("A duplicate registration keeps the first resource and destroys the second straight away")
{
	ResourceHandler handler;
	ResourceCounts firstCounts;
	ResourceCounts duplicateCounts;

	{
		ResourceHandle first = handler.RegisterLoadedResource(k_path, new CountedResource(&firstCounts));
		ResourceHandle duplicate = handler.RegisterLoadedResource(k_path, new CountedResource(&duplicateCounts));

		CHECK(duplicate.IsValid());
		CHECK(handler.GetResource<CountedResource>(duplicate) == handler.GetResource<CountedResource>(first));
		CHECK(duplicateCounts.Released == 1);
		CHECK(duplicateCounts.Deleted == 1);
		CHECK(firstCounts.Deleted == 0);
	}

	CHECK(firstCounts.Deleted == 1);
	handler.ShutDown();
}

TEST_CASE("Releasing a resource destroys what only it held straight away, and leaves what's held elsewhere")
{
	ResourceHandler handler;
	ResourceCounts heldCounts;
	ResourceCounts holderCounts;

	ResourceHandle heldElsewhere = handler.RegisterLoadedResource(k_path, new CountedResource(&heldCounts));
	{
		ResourceHandle holder = handler.RegisterLoadedResource(k_holderPath,
			new HoldingResource(&holderCounts, handler.GetHandle(k_path)));
		REQUIRE(holder.IsValid());
	}

	CHECK(holderCounts.Released == 1);
	CHECK(holderCounts.Deleted == 1);
	CHECK_FALSE(handler.HasResource(k_holderPath));
	CHECK(heldCounts.Deleted == 0);
	CHECK(handler.HasResource(k_path));

	// Now nothing else holds it, so a second holder takes it with it
	{
		ResourceHandle holder = handler.RegisterLoadedResource(k_holderPath,
			new HoldingResource(&holderCounts, std::move(heldElsewhere)));
	}

	CHECK(holderCounts.Deleted == 2);
	CHECK(heldCounts.Released == 1);
	CHECK(heldCounts.Deleted == 1);
	CHECK_FALSE(handler.HasResource(k_path));

	handler.ShutDown();
}

TEST_CASE("Shutting down destroys a resource that's still held, and what it holds, once each")
{
	ResourceHandler handler;
	ResourceCounts heldCounts;
	ResourceCounts holderCounts;

	ResourceHandle holder = handler.RegisterLoadedResource(k_holderPath,
		new HoldingResource(&holderCounts, handler.RegisterLoadedResource(k_path, new CountedResource(&heldCounts))));
	REQUIRE(holder.IsValid());

	// Releasing the holder releases what it holds while ShutDown goes through the table, in whichever order it meets them
	handler.ShutDown();
	CHECK(holderCounts.Released == 1);
	CHECK(holderCounts.Deleted == 1);
	CHECK(heldCounts.Released == 1);
	CHECK(heldCounts.Deleted == 1);
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

TEST_CASE("Paths that differ only by extension, a separator or a dot are different resources")
{
	// Each pair shared a key when keys dropped the extension and every dot, and turned separators and spaces into dots
	const std::pair<Filepath, Filepath> pairs[] = {
		{ Filepath("Assets/UnitTests/Background.jpg"), Filepath("Assets/UnitTests/Background.png") },
		{ Filepath("Assets/UnitTests/b c.png"), Filepath("Assets/UnitTests/b/c.png") },
		{ Filepath("Assets/UnitTests/v1.2/Rock.png"), Filepath("Assets/UnitTests/v12/Rock.png") },
	};

	for (const std::pair<Filepath, Filepath>& pathPair : pairs)
	{
		const Filepath& firstPath = pathPair.first;
		const Filepath& secondPath = pathPair.second;
		CAPTURE(firstPath.GetRelativePath());
		ResourceHandler handler;
		ResourceCounts firstCounts;
		ResourceCounts secondCounts;
		{
			ResourceHandle first = handler.LoadResource<CountedResource>(firstPath, &firstCounts);
			{
				ResourceHandle second = handler.LoadResource<CountedResource>(secondPath, &secondCounts);
				REQUIRE(first.IsValid());
				REQUIRE(second.IsValid());
				CHECK(handler.GetResource<CountedResource>(first) != handler.GetResource<CountedResource>(second));
				CHECK(first.GetResourcePath().GetRelativePath() == firstPath.GetRelativePath());
				CHECK(second.GetResourcePath().GetRelativePath() == secondPath.GetRelativePath());
			}

			// Releasing one leaves the other
			CHECK(secondCounts.Deleted == 1);
			CHECK(firstCounts.Deleted == 0);
			CHECK(handler.HasResource(firstPath));
			CHECK_FALSE(handler.HasResource(secondPath));
		}
		CHECK(firstCounts.Deleted == 1);
		handler.ShutDown();
	}
}

TEST_CASE("An empty path is a key like any other")
{
	ResourceHandler handler;
	CHECK_FALSE(handler.HasResource(Filepath()));
	CHECK_FALSE(handler.GetHandle(Filepath()).IsValid());
	handler.ShutDown();
}

TEST_CASE("Copied and moved handles each hold one reference, and the last one releases the resource")
{
	ResourceHandler handler;
	ResourceCounts counts;

	ResourceHandle loaded = handler.LoadResource<CountedResource>(k_path, &counts);
	ResourceHandle copied;
	copied = loaded;
	ResourceHandle moveConstructed(std::move(loaded));
	CHECK_FALSE(loaded.IsValid());
	CHECK(copied.IsValid());
	CHECK(moveConstructed.IsValid());

	copied = ResourceHandle::EmptyHandle();
	CHECK(counts.Deleted == 0);

	ResourceHandle moveAssigned;
	moveAssigned = std::move(moveConstructed);
	CHECK_FALSE(moveConstructed.IsValid());
	CHECK(counts.Deleted == 0);

	moveAssigned = ResourceHandle::EmptyHandle();
	CHECK(counts.Released == 1);
	CHECK(counts.Deleted == 1);
	CHECK_FALSE(handler.HasResource(k_path));
	handler.ShutDown();
}

TEST_CASE("Unnamed resources are separate whatever their names, and nothing finds them by name")
{
	ResourceHandler handler;
	ResourceCounts firstCounts;
	ResourceCounts secondCounts;
	ResourceCounts pathCounts;
	{
		ResourceHandle first = handler.AddUnnamedResource(std::make_unique<CountedResource>(&firstCounts), "Frame");
		ResourceHandle second = handler.AddUnnamedResource(std::make_unique<CountedResource>(&secondCounts), "Frame");
		REQUIRE(first.IsValid());
		REQUIRE(second.IsValid());
		CHECK(handler.GetResource<CountedResource>(first) != handler.GetResource<CountedResource>(second));
		CHECK(first.GetResourcePath().GetRelativePath().empty());

		// Its name is no key: a lookup by it finds nothing, and a file of that name is a resource of its own
		CHECK_FALSE(handler.HasResource(Filepath("Frame")));
		CHECK_FALSE(handler.GetHandle(Filepath("Frame")).IsValid());
		ResourceHandle fromPath = handler.LoadResource<CountedResource>(Filepath("Frame"), &pathCounts);
		CHECK(handler.GetResource<CountedResource>(fromPath) != handler.GetResource<CountedResource>(first));

		CHECK(handler.GetResourceCount() == 3);
		int unnamedCount = 0;
		handler.ForEachResource([&unnamedCount](const std::string& displayName, int referenceCount, bool isUnnamed)
			{
				CHECK(displayName == "Frame");
				CHECK(referenceCount == 1);
				unnamedCount += isUnnamed ? 1 : 0;
			});
		CHECK(unnamedCount == 2);
	}

	// Each went with its last handle
	CHECK(firstCounts.Released == 1);
	CHECK(firstCounts.Deleted == 1);
	CHECK(secondCounts.Deleted == 1);
	CHECK(pathCounts.Deleted == 1);
	CHECK(handler.GetResourceCount() == 0);
	handler.ShutDown();
}

TEST_CASE("Shutting down destroys an unnamed resource that's still held, once")
{
	ResourceHandler handler;
	ResourceCounts counts;
	{
		ResourceHandle held = handler.AddUnnamedResource(std::make_unique<CountedResource>(&counts), "Held");
		handler.ShutDown();
		CHECK(counts.Released == 1);
		CHECK(counts.Deleted == 1);
	}

	// Letting go of the handle after ShutDown doesn't release it again
	CHECK(counts.Released == 1);
	CHECK(counts.Deleted == 1);
}

TEST_CASE("Adding no unnamed resource gives an empty handle")
{
	ResourceHandler handler;
	CHECK_FALSE(handler.AddUnnamedResource(nullptr, "Nothing").IsValid());
	CHECK(handler.GetResourceCount() == 0);
	handler.ShutDown();
}
