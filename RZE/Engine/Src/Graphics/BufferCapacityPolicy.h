#pragma once

#include <Utils/PrimitiveDefs.h>

// Units are whatever the owning buffer counts in (vertices, indices, instances...).
struct BufferCapacitySettings
{
	// Never shrink below this; also the initial capacity
	U32 MinCapacity = 256;
	// Update() calls per shrink-evaluation window
	U32 ShrinkWindowCalls = 300;
	// Shrink only once recent peak usage is at or below 1/ShrinkUsageDivisor of capacity
	U32 ShrinkUsageDivisor = 4;
};

// Decides when a reusable buffer should be reallocated. Pure CPU bookkeeping; owns no GPU resources.
//
// Grow: immediately, to the next power of two, so total reallocations under growth are O(log n).
// Shrink: only at window boundaries, when the peak across the last two windows (a sliding max covering
// between ShrinkWindowCalls and 2x that) is <= 1/ShrinkUsageDivisor of capacity. Shrinks to 2x that
// peak, leaving usage at <= 50% - well clear of both the grow (100%) and shrink (25%) thresholds, so
// brief spikes or toggling don't cause grow/shrink churn.
class BufferCapacityPolicy
{
public:
	explicit BufferCapacityPolicy(const BufferCapacitySettings& settings = BufferCapacitySettings());

	// Records one use of needed elements. Returns true if GetCapacity() changed and the buffer must be reallocated.
	bool Update(U32 needed);

	U32 GetCapacity() const { return m_capacity; }
	const BufferCapacitySettings& GetSettings() const { return m_settings; }

private:
	BufferCapacitySettings m_settings;
	U32 m_capacity = 0;

	// Two-bucket sliding max of usage
	U32 m_peakThisWindow = 0;
	U32 m_peakLastWindow = 0;
	U32 m_callsInWindow = 0;
};
