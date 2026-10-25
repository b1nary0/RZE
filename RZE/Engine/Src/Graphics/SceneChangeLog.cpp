#include <StdAfx.h>
#include <Graphics/SceneChangeLog.h>

#include <algorithm>

void SceneChangeLog::Add(U64 revision, const BoundingBox& bounds)
{
	if (bounds.IsEmpty())
	{
		return;
	}

	if (m_changes.size() == k_capacity)
	{
		Forget(m_changes.front().Revision);
		m_changes.pop_front();
	}
	m_changes.push_back({ revision, bounds });
}

void SceneChangeLog::AddEverywhere(U64 revision)
{
	m_changes.clear();
	Forget(revision);
}

bool SceneChangeLog::HasChangedWithin(const BoundingBox& region, U64 sinceRevision) const
{
	// Something it may have seen was forgotten, so where it changed isn't known
	if (sinceRevision < m_forgottenRevision)
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

void SceneChangeLog::Forget(U64 revision)
{
	m_forgottenRevision = std::max(m_forgottenRevision, revision);
}
