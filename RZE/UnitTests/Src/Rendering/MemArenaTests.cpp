#include <doctest/doctest.h>

#include <Rendering/MemArena.h>

#include <Utils/PrimitiveDefs.h>

#include <algorithm>
#include <cstdint>
#include <cstring>

namespace
{
	constexpr size_t k_arenaSize = 64;

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
