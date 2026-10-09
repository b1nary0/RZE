#pragma once

#include <Utils/PrimitiveDefs.h>
#include <Utils/Interfaces/Resource.h>
#include <Utils/Platform/Timers/HiResTimer.h>

// Per-frame allowance for main-thread work performed by async operations. One budget is shared by
// every operation ticked in a frame (see AsyncOperationManager::Tick).
//
// - Time and upload bytes are soft limits: the first unit of work each frame is always allowed so
//   operations are guaranteed to make progress, even if that unit is larger than the budget.
// - Arena bytes are a hard limit: Rendering::MemArena asserts if a frame overflows it.
class AsyncFrameBudget
{
public:
	AsyncFrameBudget(double timeBudgetMS, size_t uploadBudgetBytes, size_t arenaBudgetBytes, size_t arenaCapacityBytes);

public:
	// True once the soft limits have been spent. Work that has no byte cost should stop when this is true.
	bool IsExhausted() const;

	// Attempts to reserve the cost of one unit of work. Returns false if the work should wait for a later frame.
	bool TryConsume(const ResourceFinalizeCost& cost);

	// False if the cost could never fit in the arena, even on an otherwise empty frame.
	bool CanEverAfford(const ResourceFinalizeCost& cost) const;

	size_t GetArenaBytesConsumed() const { return m_arenaBytesConsumed; }

private:
	double GetElapsedMS() const;

private:
	mutable HiResTimer m_timer;

	double m_timeBudgetMS;
	size_t m_uploadBytesRemaining;
	size_t m_arenaBytesRemaining;
	size_t m_arenaCapacityBytes;
	size_t m_arenaBytesConsumed = 0;

	bool m_hasConsumedWork = false;
};
