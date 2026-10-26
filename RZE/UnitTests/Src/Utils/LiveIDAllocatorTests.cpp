#include <doctest/doctest.h>

#include <Utils/LiveIDAllocator.h>

namespace
{
	// An ID's slot is its low half (see LiveIDAllocator)
	U32 GetSlot(U64 id)
	{
		return static_cast<U32>(id);
	}
}

TEST_CASE("A new ID is alive until it's released")
{
	LiveIDAllocator allocator;
	const U64 first = allocator.Acquire();
	const U64 second = allocator.Acquire();
	CHECK(first != second);
	CHECK(allocator.Contains(first));
	CHECK(allocator.Contains(second));

	allocator.Release(first);
	CHECK_FALSE(allocator.Contains(first));
	CHECK(allocator.Contains(second));
}

TEST_CASE("A released ID's slot goes to the next ID, which the old ID doesn't match")
{
	LiveIDAllocator allocator;
	const U64 old = allocator.Acquire();
	allocator.Release(old);

	const U64 reused = allocator.Acquire();
	CHECK(GetSlot(reused) == GetSlot(old));
	CHECK(reused != old);
	CHECK(allocator.Contains(reused));
	CHECK_FALSE(allocator.Contains(old));
}

TEST_CASE("Freed slots are reused most recent first, then new ones are added")
{
	LiveIDAllocator allocator;
	const U64 a = allocator.Acquire();
	const U64 b = allocator.Acquire();
	const U64 c = allocator.Acquire();
	allocator.Release(a);
	allocator.Release(c);

	CHECK(GetSlot(allocator.Acquire()) == GetSlot(c));
	CHECK(GetSlot(allocator.Acquire()) == GetSlot(a));
	const U64 added = allocator.Acquire();
	CHECK(GetSlot(added) != GetSlot(a));
	CHECK(GetSlot(added) != GetSlot(b));
	CHECK(GetSlot(added) != GetSlot(c));
}

TEST_CASE("Slots follow the most IDs alive at once, not how many were handed out")
{
	LiveIDAllocator allocator;
	const U64 kept = allocator.Acquire();
	const U64 first = allocator.Acquire();
	allocator.Release(first);

	for (int i = 0; i < 1000; ++i)
	{
		const U64 id = allocator.Acquire();
		CHECK(GetSlot(id) == GetSlot(first));
		allocator.Release(id);
	}

	CHECK(allocator.Contains(kept));
	CHECK_FALSE(allocator.Contains(first));
}

TEST_CASE("The invalid ID, and IDs for slots that don't exist, are never alive")
{
	LiveIDAllocator allocator;
	CHECK_FALSE(allocator.Contains(LiveIDAllocator::k_invalidID));
	CHECK_FALSE(allocator.Contains(0));

	allocator.Acquire();
	CHECK_FALSE(allocator.Contains(LiveIDAllocator::k_invalidID));
	// Slot 1, which hasn't been handed out
	CHECK_FALSE(allocator.Contains(1));
}

TEST_CASE("A slot whose generations run out is retired rather than reused")
{
	// Each slot can be handed out twice: generations 0 and 1
	LiveIDAllocator allocator(1);
	const U64 first = allocator.Acquire();
	allocator.Release(first);
	const U64 second = allocator.Acquire();
	REQUIRE(GetSlot(second) == GetSlot(first));
	allocator.Release(second);

	const U64 third = allocator.Acquire();
	CHECK(GetSlot(third) != GetSlot(first));
	CHECK(allocator.Contains(third));
	CHECK_FALSE(allocator.Contains(first));
	CHECK_FALSE(allocator.Contains(second));
}
