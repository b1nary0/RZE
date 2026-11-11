#include <doctest/doctest.h>

#include <Rendering/MemArena.h>

#include <Utils/Memory/MemoryUtils.h>
#include <Utils/PrimitiveDefs.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>

namespace
{
	constexpr size_t k_arenaSize = 64;

	// Shaped like a create command: a shared_ptr, a pointer and a size
	struct TestCommand
	{
		std::shared_ptr<int> bufferPtr;
		void* data = nullptr;
		size_t count = 0;
	};

	bool IsAligned(const void* pointer, size_t alignment)
	{
		return reinterpret_cast<std::uintptr_t>(pointer) % alignment == 0;
	}

	bool IsInside(const void* pointer, const Byte* start, size_t size)
	{
		const std::uintptr_t address = reinterpret_cast<std::uintptr_t>(pointer);
		const std::uintptr_t begin = reinterpret_cast<std::uintptr_t>(start);
		return address >= begin && address < begin + size;
	}

	bool IsFilledWith(const Byte* memory, size_t size, Byte value)
	{
		return std::all_of(memory, memory + size, [value](Byte byte) { return byte == value; });
	}
}

TEST_CASE("Arena allocations that fit sit back to back")
{
	Rendering::MemArena::InitializeArena(k_arenaSize);

	Byte* const first = static_cast<Byte*>(Rendering::MemArena::Alloc(16));
	Byte* const second = static_cast<Byte*>(Rendering::MemArena::Alloc(48));
	CHECK(second == first + 16);

	Rendering::MemArena::Shutdown();
}

TEST_CASE("An allocation the arena can't fit comes from the heap, zeroed, and leaves the arena alone")
{
	Rendering::MemArena::InitializeArena(k_arenaSize);

	Byte* const arena = static_cast<Byte*>(Rendering::MemArena::Alloc(48));
	memset(arena, 0xAA, 48);

	Byte* const spilled = static_cast<Byte*>(Rendering::MemArena::Alloc(32));
	REQUIRE(spilled != nullptr);
	CHECK_FALSE(IsInside(spilled, arena, k_arenaSize));
	CHECK(IsFilledWith(spilled, 32, 0));

	memset(spilled, 0xBB, 32);
	CHECK(IsFilledWith(arena, 48, 0xAA));

	// The spill didn't use up the arena: what's left of it still fits
	Byte* const rest = static_cast<Byte*>(Rendering::MemArena::Alloc(16));
	CHECK(rest == arena + 48);

	Rendering::MemArena::Shutdown();
}

TEST_CASE("The arena counts what overflowed and carries on")
{
	Rendering::MemArena::InitializeArena(k_arenaSize);

	Rendering::MemArena::Alloc(48);
	Rendering::MemArena::Alloc(80);
	Rendering::MemArena::Cycle();

	CHECK(Rendering::MemArena::GetTotalMemoryAllocatedLastFrame() == 128);
	CHECK(Rendering::MemArena::GetPeakUsedBytes() == 128);
	CHECK(Rendering::MemArena::GetPressureValue() == Rendering::MemArena::PressureValue::HIGH);

	// The other buffer, then the one that overflowed again, which frees its spill: both start empty
	for (int frame = 0; frame < 2; ++frame)
	{
		Byte* const first = static_cast<Byte*>(Rendering::MemArena::Alloc(32));
		Byte* const second = static_cast<Byte*>(Rendering::MemArena::Alloc(32));
		CHECK(second == first + 32);
		Rendering::MemArena::Cycle();
	}

	CHECK(Rendering::MemArena::GetTotalMemoryAllocatedLastFrame() == 64);
	CHECK(Rendering::MemArena::GetPeakUsedBytes() == 128);

	Rendering::MemArena::Shutdown();
}

TEST_CASE("Arena pressure is the last frame's")
{
	Rendering::MemArena::InitializeArena(100);

	Rendering::MemArena::Alloc(70);
	Rendering::MemArena::Cycle();
	CHECK(Rendering::MemArena::GetPressureValue() == Rendering::MemArena::PressureValue::MED);

	Rendering::MemArena::Alloc(95);
	Rendering::MemArena::Cycle();
	CHECK(Rendering::MemArena::GetPressureValue() == Rendering::MemArena::PressureValue::HIGH);

	Rendering::MemArena::Alloc(10);
	Rendering::MemArena::Cycle();
	CHECK(Rendering::MemArena::GetPressureValue() == Rendering::MemArena::PressureValue::LOW);

	Rendering::MemArena::Shutdown();
}

TEST_CASE("A command after odd-sized data is aligned for its type")
{
	Rendering::MemArena::InitializeArena(256);

	// Like the index data of a mesh with an odd number of triangles
	Byte* const data = static_cast<Byte*>(Rendering::MemArena::Alloc(4 * 3));
	TestCommand* const command = Rendering::MemArena::AllocType<TestCommand>();

	CHECK(IsAligned(command, alignof(TestCommand)));
	CHECK(reinterpret_cast<Byte*>(command) == data + MemoryUtils::AlignSize(12, alignof(TestCommand)));

	Rendering::MemArena::Shutdown();
}

TEST_CASE("Raw arena data is aligned for any type")
{
	Rendering::MemArena::InitializeArena(256);

	Byte* const first = static_cast<Byte*>(Rendering::MemArena::Alloc(3));
	Byte* const second = static_cast<Byte*>(Rendering::MemArena::Alloc(8));

	CHECK(IsAligned(first, alignof(std::max_align_t)));
	CHECK(IsAligned(second, alignof(std::max_align_t)));
	CHECK(second == first + alignof(std::max_align_t));

	Rendering::MemArena::Shutdown();
}

TEST_CASE("Arena padding counts towards the frame's total")
{
	Rendering::MemArena::InitializeArena(256);

	Rendering::MemArena::Alloc(3);
	Rendering::MemArena::Alloc(8);
	Rendering::MemArena::Cycle();

	CHECK(Rendering::MemArena::GetTotalMemoryAllocatedLastFrame() == alignof(std::max_align_t) + 8);

	Rendering::MemArena::Shutdown();
}

TEST_CASE("An allocation that only its padding pushes past the end of the arena spills")
{
	Rendering::MemArena::InitializeArena(k_arenaSize);

	// 60 + 4 would just fit, but aligning the 4 puts it at 64
	Byte* const arena = static_cast<Byte*>(Rendering::MemArena::Alloc(60));
	Byte* const spilled = static_cast<Byte*>(Rendering::MemArena::Alloc(4));

	CHECK_FALSE(IsInside(spilled, arena, k_arenaSize));
	CHECK(IsAligned(spilled, alignof(std::max_align_t)));

	// The spill isn't padded
	Rendering::MemArena::Cycle();
	CHECK(Rendering::MemArena::GetTotalMemoryAllocatedLastFrame() == 64);

	Rendering::MemArena::Shutdown();
}

TEST_CASE("Arena allocations are zeroed, even where an earlier frame wrote")
{
	Rendering::MemArena::InitializeArena(k_arenaSize);

	// The arena's buffers start uninitialized, so only Alloc's zeroing can make this hold
	Byte* const first = static_cast<Byte*>(Rendering::MemArena::Alloc(k_arenaSize));
	CHECK(IsFilledWith(first, k_arenaSize, 0));
	memset(first, 0xCD, k_arenaSize);

	// Back to the same buffer two cycles later
	Rendering::MemArena::Cycle();
	Rendering::MemArena::Cycle();
	Byte* const again = static_cast<Byte*>(Rendering::MemArena::Alloc(k_arenaSize));
	REQUIRE(again == first);
	CHECK(IsFilledWith(again, k_arenaSize, 0));

	Rendering::MemArena::Shutdown();
}
