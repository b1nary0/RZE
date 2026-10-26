#pragma once

#include <Utils/PrimitiveDefs.h>

#include <vector>

// Hands out IDs for objects and tracks which are still alive, so a handle holding an object's ID can tell once the
// object is destroyed, even if a new object lands at its address (e.g. GameObjectLifetime). No ID is handed out twice.
//
// An ID is a slot index (low 32 bits) and that slot's generation (high 32 bits). Each live object holds one slot;
// releasing it bumps the slot's generation, so the old ID stops matching, and frees the slot for the next Acquire.
// A slot whose generations run out is retired rather than reused. Memory follows the most objects ever alive at
// once, 8 bytes each, not how many were ever created, and a lookup is a single array read rather than a hash lookup.
// Main thread only: the slots move when they grow.
class LiveIDAllocator
{
public:
	// Never handed out
	static constexpr U64 k_invalidID = ~0ull;
	// The last generation a slot can have before it's retired
	static constexpr U32 k_maxGeneration = 0xFFFFFFFF;

public:
	// maxGeneration below k_maxGeneration retires slots sooner; for tests
	explicit LiveIDAllocator(U32 maxGeneration = k_maxGeneration);

	LiveIDAllocator(const LiveIDAllocator&) = delete;
	LiveIDAllocator& operator=(const LiveIDAllocator&) = delete;

	// A new ID, alive until it's released
	U64 Acquire();
	// The ID must be alive
	void Release(U64 id);

	bool Contains(U64 id) const
	{
		const U32 index = GetLow(id);
		return index < m_slots.size() && m_slots[index] == Pack(GetGeneration(id), k_alive);
	}

private:
	// IDs and slots share a layout: the generation in the high half; an ID's low half is its slot index, and a slot's
	// is its state (k_alive, k_retired, or while it's free the next free slot's index)
	static U64 Pack(U32 generation, U32 low) { return (static_cast<U64>(generation) << 32) | low; }
	static U32 GetGeneration(U64 value) { return static_cast<U32>(value >> 32); }
	static U32 GetLow(U64 value) { return static_cast<U32>(value); }

private:
	static constexpr U32 k_alive = 0xFFFFFFFF;
	static constexpr U32 k_retired = 0xFFFFFFFE;
	// The last free slot's state, and m_freeHead when there's none. Also caps the slot count, so no index is ever
	// one of these states.
	static constexpr U32 k_endOfFreeList = 0xFFFFFFFD;

private:
	std::vector<U64> m_slots;
	// The most recently freed slot, so it's reused while still in cache
	U32 m_freeHead = k_endOfFreeList;
	const U32 m_maxGeneration;
};
