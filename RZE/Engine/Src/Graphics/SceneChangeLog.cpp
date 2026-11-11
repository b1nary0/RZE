#include <Graphics/SceneChangeLog.h>

#include <algorithm>
#include <cstddef>

void SceneChangeLog::Add(U64 revision, const BoundingBox& bounds)
{
	if (bounds.IsEmpty())
	{
		return;
	}

	if (m_changes.size() == k_capacity)
	{
		MergeCheapestPair();
	}

	m_changes.push_back({ revision, bounds });
	if (m_changes.size() >= 2)
	{
		m_mergeCosts.push_back(CalculateMergeCost(m_changes.size() - 2));
	}
}

void SceneChangeLog::AddEverywhere(U64 revision)
{
	m_changes.clear();
	m_mergeCosts.clear();
	Forget(revision);
}

bool SceneChangeLog::HasChangedWithin(const BoundingBox& region, U64 sinceRevision) const
{
	// Something it may have seen was forgotten, so where it changed isn't known
	if (HasForgottenSince(sinceRevision))
	{
		return true;
	}

	// Newest first, stopping at the changes it already saw
	for (auto iter = m_changes.rbegin(); iter != m_changes.rend() && iter->Revision > sinceRevision; ++iter)
	{
		if (iter->Bounds.Intersects(region))
		{
			return true;
		}
	}
	return false;
}

bool SceneChangeLog::ForEachChangeSince(U64 sinceRevision, const std::function<void(const BoundingBox&)>& visit) const
{
	if (HasForgottenSince(sinceRevision))
	{
		return false;
	}

	for (auto iter = m_changes.rbegin(); iter != m_changes.rend() && iter->Revision > sinceRevision; ++iter)
	{
		visit(iter->Bounds);
	}
	return true;
}

void SceneChangeLog::MergeCheapestPair()
{
	AssertExpr(m_changes.size() >= 2);

	// Only neighbours in the log, so it stays in revision order and finding them stays cheap. Neighbours are usually
	// close in the scene too: an object's old and new bounds, or objects changed one after another.
	const size_t cheapest = static_cast<size_t>(std::min_element(m_mergeCosts.begin(), m_mergeCosts.end()) - m_mergeCosts.begin());

	// The newer takes in the older, so the merged change counts for anything that hadn't seen either
	m_changes[cheapest + 1].Bounds.Grow(m_changes[cheapest].Bounds);
	m_changes.erase(m_changes.begin() + static_cast<std::ptrdiff_t>(cheapest));
	m_mergeCosts.erase(m_mergeCosts.begin() + static_cast<std::ptrdiff_t>(cheapest));

	// The merged change is at cheapest now: the pairs on either side of it changed
	if (cheapest > 0)
	{
		m_mergeCosts[cheapest - 1] = CalculateMergeCost(cheapest - 1);
	}
	if (cheapest < m_mergeCosts.size())
	{
		m_mergeCosts[cheapest] = CalculateMergeCost(cheapest);
	}
}

float SceneChangeLog::CalculateMergeCost(size_t index) const
{
	const BoundingBox& newer = m_changes[index + 1].Bounds;
	BoundingBox merged = m_changes[index].Bounds;
	merged.Grow(newer);
	return merged.GetSurfaceArea() - newer.GetSurfaceArea();
}

void SceneChangeLog::Forget(U64 revision)
{
	m_forgottenRevision = std::max(m_forgottenRevision, revision);
}

bool SceneChangeLog::HasForgottenSince(U64 sinceRevision) const
{
	return sinceRevision < m_forgottenRevision;
}
