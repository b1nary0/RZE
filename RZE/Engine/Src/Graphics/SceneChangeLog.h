#pragma once

#include <Utils/Math/BoundingBox.h>
#include <Utils/PrimitiveDefs.h>

#include <deque>

// Where a scene's render objects changed, and when (by RenderScene::GetRevision), so something cached from part of
// the scene (e.g. a point light's shadow cube) can tell whether that part changed since it was made. Only the latest
// k_capacity changes are kept: asking about a revision from before them answers that it changed.
class SceneChangeLog
{
public:
	// Enough for a couple of hundred objects moving in a frame (each logs where it was and where it is)
	static constexpr size_t k_capacity = 256;

public:
	// Something within bounds changed at revision, which is never lower than the last change's. Empty bounds are
	// ignored.
	void Add(U64 revision, const BoundingBox& bounds);
	// Everything changed at revision (e.g. every object was removed): anything older is taken as changed
	void AddEverywhere(U64 revision);

	// Whether anything intersecting region changed after sinceRevision. True when the log doesn't go back that far.
	bool HasChangedWithin(const BoundingBox& region, U64 sinceRevision) const;

private:
	// Changes at or before this revision are forgotten, so asking about anything older answers that it changed
	void Forget(U64 revision);

private:
	struct Change
	{
		U64 Revision;
		BoundingBox Bounds;
	};

	// Oldest first
	std::deque<Change> m_changes;
	U64 m_forgottenRevision = 0;
};
