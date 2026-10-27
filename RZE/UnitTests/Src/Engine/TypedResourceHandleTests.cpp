#include <doctest/doctest.h>

#include <EngineCore/Resources/ResourceHandler.h>
#include <EngineCore/Resources/TypedResourceHandle.h>

#include <Utils/Interfaces/Resource.h>
#include <Utils/Platform/Filepath.h>

#include <type_traits>
#include <utility>

namespace
{
	// Loads without touching a file, and counts its deletion
	class CountedResource : public IResource
	{
	public:
		explicit CountedResource(int* deleted)
			: m_deleted(deleted) {}
		~CountedResource() override { ++*m_deleted; }

		bool Load(const Filepath& filePath) override { return true; }
		void Release() override {}

	private:
		int* m_deleted;
	};

	using CountedHandle = TypedResourceHandle<CountedResource>;

	const Filepath k_path("Assets/UnitTests/Typed.resource");
}

static_assert(!std::is_copy_constructible_v<CountedHandle> && !std::is_copy_assignable_v<CountedHandle>, "One reference per holder");
static_assert(std::is_move_constructible_v<CountedHandle> && std::is_move_assignable_v<CountedHandle>, "Holders can hand it over");

TEST_CASE("An empty typed handle holds nothing")
{
	const CountedHandle empty;
	CHECK_FALSE(empty.IsValid());
	CHECK(empty.Get() == nullptr);
	CHECK_FALSE(empty.GetHandle().IsValid());

	const CountedHandle fromEmpty(ResourceHandle::EmptyHandle());
	CHECK_FALSE(fromEmpty.IsValid());
}

TEST_CASE("A typed handle resolves its resource and keeps it loaded until it lets go")
{
	ResourceHandler handler;
	int deleted = 0;
	{
		CountedHandle typed(handler.LoadResource<CountedResource>(k_path, &deleted));
		REQUIRE(typed.IsValid());
		CHECK(typed.Get() == handler.GetResource<CountedResource>(typed.GetHandle()));
		CHECK(handler.HasResource(k_path));

		typed.Reset();
		CHECK_FALSE(typed.IsValid());
		CHECK(deleted == 1);
		CHECK_FALSE(handler.HasResource(k_path));

		typed = CountedHandle(handler.LoadResource<CountedResource>(k_path, &deleted));
		CHECK(typed.IsValid());
	}
	// Destroying it lets go too
	CHECK(deleted == 2);
	handler.ShutDown();
}

TEST_CASE("Moving a typed handle hands its reference over and leaves the source empty")
{
	ResourceHandler handler;
	int deleted = 0;
	CountedHandle source(handler.LoadResource<CountedResource>(k_path, &deleted));
	const CountedResource* const resource = source.Get();

	CountedHandle moved = std::move(source);
	CHECK(moved.Get() == resource);
	CHECK_FALSE(source.IsValid());
	CHECK(source.Get() == nullptr);

	CountedHandle assigned;
	assigned = std::move(moved);
	CHECK(assigned.Get() == resource);
	CHECK(moved.Get() == nullptr);
	CHECK(deleted == 0);

	assigned.Reset();
	CHECK(deleted == 1);
	handler.ShutDown();
}

TEST_CASE("A typed handle made from a shared handle keeps the resource after the other lets go")
{
	ResourceHandler handler;
	int deleted = 0;
	CountedHandle typed;
	{
		const ResourceHandle shared = handler.LoadResource<CountedResource>(k_path, &deleted);
		typed = CountedHandle(shared);
		CHECK(typed.Get() == handler.GetResource<CountedResource>(shared));
	}
	CHECK(deleted == 0);
	CHECK(typed.IsValid());

	typed.Reset();
	CHECK(deleted == 1);
	handler.ShutDown();
}
