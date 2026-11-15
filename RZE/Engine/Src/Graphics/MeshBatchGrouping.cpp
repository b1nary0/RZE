#include <Graphics/MeshBatchGrouping.h>

#include <Graphics/MeshDraw.h>

#include <functional>

void MeshBatchGrouping::Clear()
{
	m_entries.clear();
}

void MeshBatchGrouping::Add(const MeshDraw& draw, const void* bindKey)
{
	m_entries.push_back({ &draw, bindKey, true });
}

void MeshBatchGrouping::AddInOrder(const MeshDraw& draw, const void* bindKey)
{
	m_entries.push_back({ &draw, bindKey, false });
}

void MeshBatchGrouping::Build()
{
	m_batches.clear();
	m_placements.clear();
	m_groupBatches.clear();
	m_placements.reserve(m_entries.size());

	// Each entry's batch (in its placement's Instance for now), a batch made where its first entry is, and how many
	// instances each has. A stable counting sort: no keys are compared for order, so the order is the same every run.
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
			}
			lastInOrder = &entry;
		}

		++m_batches[batchIndex].InstanceCount;
		m_placements.push_back({ entry.Draw, batchIndex });
	}

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

size_t MeshBatchGrouping::GroupKeyHash::operator()(const GroupKey& key) const
{
	const size_t bindKeyHash = std::hash<const void*>()(key.BindKey);
	const size_t geometryHash = std::hash<const void*>()(key.Geometry);
	// Mixes the two, so swapping them, or keys differing in only one, hash differently
	return bindKeyHash ^ (geometryHash + 0x9e3779b97f4a7c15ull + (bindKeyHash << 6) + (bindKeyHash >> 2));
}
