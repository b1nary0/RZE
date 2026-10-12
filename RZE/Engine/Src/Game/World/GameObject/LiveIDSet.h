#pragma once

#include <Utils/PrimitiveDefs.h>

#include <vector>

// The set of live IDs, for IDs that are handed out in increasing order and never reused (GameObjectLifetime).
// One bit per ID ever handed out, so a lookup is a single array read rather than a hash lookup; it only grows,
// by 8 KB per 65536 IDs.
class LiveIDSet
{
public:
	void Add(U32 id)
	{
		const size_t word = id >> 6;
		if (word >= m_words.size())
		{
			m_words.resize(word + 1, 0);
		}
		m_words[word] |= Bit(id);
	}

	void Remove(U32 id)
	{
		const size_t word = id >> 6;
		if (word < m_words.size())
		{
			m_words[word] &= ~Bit(id);
		}
	}

	bool Contains(U32 id) const
	{
		const size_t word = id >> 6;
		return word < m_words.size() && (m_words[word] & Bit(id)) != 0;
	}

private:
	static U64 Bit(U32 id) { return U64(1) << (id & 63); }

private:
	std::vector<U64> m_words;
};
