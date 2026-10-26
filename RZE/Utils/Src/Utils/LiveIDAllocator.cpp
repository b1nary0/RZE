#include <StdAfx.h>
#include <Utils/LiveIDAllocator.h>

#include <Utils/DebugUtils/Debug.h>

LiveIDAllocator::LiveIDAllocator(U32 maxGeneration)
	: m_maxGeneration(maxGeneration)
{
}

U64 LiveIDAllocator::Acquire()
{
	if (m_freeHead == k_endOfFreeList)
	{
		AssertMsg(m_slots.size() < k_endOfFreeList, "Out of live ID slots");
		m_freeHead = static_cast<U32>(m_slots.size());
		m_slots.push_back(Pack(0, k_endOfFreeList));
	}

	const U32 index = m_freeHead;
	const U32 generation = GetGeneration(m_slots[index]);
	m_freeHead = GetLow(m_slots[index]);
	m_slots[index] = Pack(generation, k_alive);
	return Pack(generation, index);
}

void LiveIDAllocator::Release(U64 id)
{
	const bool isAlive = Contains(id);
	AssertMsg(isAlive, "Releasing an ID that isn't alive");
	if (!isAlive)
	{
		return;
	}

	const U32 index = GetLow(id);
	const U32 generation = GetGeneration(id);
	if (generation == m_maxGeneration)
	{
		// Reusing it would hand out an ID that's been handed out before
		m_slots[index] = Pack(generation, k_retired);
		return;
	}

	m_slots[index] = Pack(generation + 1, m_freeHead);
	m_freeHead = index;
}
