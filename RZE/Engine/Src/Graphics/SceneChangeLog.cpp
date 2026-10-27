#include <StdAfx.h>
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
		MergeClosestPair();
	}

	m_changes.push_back({ revision, bounds });
	if (m_changes.size() >= 2)
	{
		m_mergedAreas.push_back(CalculateMergedArea(m_changes.size() - 2));
	}
}

void SceneChangeLog::AddEverywhere(U64 revision)
{
	m_changes.clear();
	m_mergedAreas.clear();
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

void SceneChangeLog::MergeClosestPair()
{
	AssertExpr(m_changes.size() >= 2);

	// Only neighbours in the log, so it stays in revision order and finding them stays cheap. Neighbours are usually
	// close in the scene too: an object's old and new bounds, or objects changed one after another.
	const size_t closest = static_cast<size_t>(std::min_element(m_mergedAreas.begin(), m_mergedAreas.end()) - m_mergedAreas.begin());

	// The newer takes in the older, so the merged change counts for anything that hadn't seen either
	m_changes[closest + 1].Bounds.Grow(m_changes[closest].Bounds);
	m_changes.erase(m_changes.begin() + static_cast<std::ptrdiff_t>(closest));
	m_mergedAreas.erase(m_mergedAreas.begin() + static_cast<std::ptrdiff_t>(closest));

	// The merged change is at closest now: the pairs on either side of it changed
	if (closest > 0)
	{
		m_mergedAreas[closest - 1] = CalculateMergedArea(closest - 1);
	}
	if (closest < m_mergedAreas.size())
	{
		m_mergedAreas[closest] = CalculateMergedArea(closest);
	}
}

float SceneChangeLog::CalculateMergedArea(size_t index) const
{
	BoundingBox merged = m_changes[index].Bounds;
	merged.Grow(m_changes[index + 1].Bounds);
	return merged.GetSurfaceArea();
}

void SceneChangeLog::Forget(U64 revision)
{
	m_forgottenRevision = std::max(m_forgottenRevision, revision);
}

bool SceneChangeLog::HasForgottenSince(U64 sinceRevision) const
{
	return sinceRevision < m_forgottenRevision;
}
