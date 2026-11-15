#include <Graphics/MeshBatchGrouping.h>

#include <Graphics/MeshDraw.h>

#include <algorithm>
#include <functional>
#include <numeric>

void MeshBatchGrouping::Clear()
{
	m_entries.clear();
}

void MeshBatchGrouping::Add(const MeshDraw& draw, const void* bindKey, const void* orderKey)
{
	m_entries.push_back({ &draw, bindKey, orderKey, true });
}

void MeshBatchGrouping::AddInOrder(const MeshDraw& draw, const void* bindKey)
{
	m_entries.push_back({ &draw, bindKey, nullptr, false });
}

void MeshBatchGrouping::Build()
{
	m_batches.clear();
	m_placements.clear();
	m_groupBatches.clear();
	m_orderKeyRanks.clear();
	m_bindKeyRanks.clear();
	m_batchRanks.clear();
	m_placements.reserve(m_entries.size());

	// Each entry's batch (in its placement's Instance for now), a batch made where its first entry is, and how many
	// instances each has. Then the batches are sorted by their keys' ranks, and the instances placed with a stable
	// counting sort: no addresses are compared for order, so the order is the same every run.
	// The entry before, if it was added in order: the one an in-order entry with the same key can join
	const Entry* lastInOrder = nullptr;
	for (const Entry& entry : m_entries)
	{
		U32 batchIndex = static_cast<U32>(m_batches.size());
		if (entry.IsGrouped)
		{
			const auto [it, isNew] = m_groupBatches.try_emplace(GroupKey{ entry.BindKey, entry.Draw->Geometry }, batchIndex);
			batchIndex = it->second;
			if (isNew)
			{
				m_batches.push_back({ entry.Draw, 0, 0 });

				// A key's rank is its first batch's place among the grouped batches with a new key
				BatchRank& rank = m_batchRanks.emplace_back();
				rank.OrderKey = m_orderKeyRanks.try_emplace(entry.OrderKey, static_cast<U32>(m_orderKeyRanks.size())).first->second;
				rank.BindKey = m_bindKeyRanks.try_emplace(entry.BindKey, static_cast<U32>(m_bindKeyRanks.size())).first->second;
			}
			lastInOrder = nullptr;
		}
		else
		{
			const bool joinsLast = lastInOrder != nullptr && lastInOrder->BindKey == entry.BindKey && lastInOrder->Draw->Geometry == entry.Draw->Geometry;
			if (joinsLast)
			{
				// The last in-order entry's batch is the newest: nothing was added after it
				batchIndex = static_cast<U32>(m_batches.size() - 1);
			}
			else
			{
				m_batches.push_back({ entry.Draw, 0, 0 });
				m_batchRanks.push_back({ 0, 0, false });
			}
			lastInOrder = &entry;
		}

		++m_batches[batchIndex].InstanceCount;
		m_placements.push_back({ entry.Draw, batchIndex });
	}

	SortGroupedBatches();

	// Each batch's instances follow the last's. The counts start again from 0, to count the instances placed.
	U32 instanceCount = 0;
	for (Batch& batch : m_batches)
	{
		batch.FirstInstance = instanceCount;
		instanceCount += batch.InstanceCount;
		batch.InstanceCount = 0;
	}

	// Each entry's instance in the next place in its batch
	for (Placement& placement : m_placements)
	{
		Batch& batch = m_batches[placement.Instance];
		placement.Instance = batch.FirstInstance + batch.InstanceCount;
		++batch.InstanceCount;
	}
}

void MeshBatchGrouping::SortGroupedBatches()
{
	const U32 batchCount = static_cast<U32>(m_batches.size());
	m_sortedBatchIndices.resize(batchCount);
	std::iota(m_sortedBatchIndices.begin(), m_sortedBatchIndices.end(), 0u);

	// Each run of grouped batches, between in-order ones, sorts on its own. Stable, so batches of equal keys keep the
	// order their first draws were added in.
	const auto byRank = [this](U32 a, U32 b)
		{
			const BatchRank& rankA = m_batchRanks[a];
			const BatchRank& rankB = m_batchRanks[b];
			return rankA.OrderKey != rankB.OrderKey ? rankA.OrderKey < rankB.OrderKey : rankA.BindKey < rankB.BindKey;
		};
	U32 runStart = 0;
	while (runStart < batchCount)
	{
		if (!m_batchRanks[runStart].IsGrouped)
		{
			++runStart;
			continue;
		}

		U32 runEnd = runStart + 1;
		while (runEnd < batchCount && m_batchRanks[runEnd].IsGrouped)
		{
			++runEnd;
		}
		std::stable_sort(m_sortedBatchIndices.begin() + runStart, m_sortedBatchIndices.begin() + runEnd, byRank);
		runStart = runEnd;
	}

	m_sortedBatches.clear();
	m_newBatchIndices.resize(batchCount);
	for (U32 newIndex = 0; newIndex < batchCount; ++newIndex)
	{
		const U32 oldIndex = m_sortedBatchIndices[newIndex];
		m_sortedBatches.push_back(m_batches[oldIndex]);
		m_newBatchIndices[oldIndex] = newIndex;
	}
	m_batches.swap(m_sortedBatches);

	// Each placement holds its batch's index until its instance is placed
	for (Placement& placement : m_placements)
	{
		placement.Instance = m_newBatchIndices[placement.Instance];
	}
}

size_t MeshBatchGrouping::GroupKeyHash::operator()(const GroupKey& key) const
{
	const size_t bindKeyHash = std::hash<const void*>()(key.BindKey);
	const size_t geometryHash = std::hash<const void*>()(key.Geometry);
	// Mixes the two, so swapping them, or keys differing in only one, hash differently
	return bindKeyHash ^ (geometryHash + 0x9e3779b97f4a7c15ull + (bindKeyHash << 6) + (bindKeyHash >> 2));
}
