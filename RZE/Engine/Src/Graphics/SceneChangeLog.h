#pragma once

#include <Utils/Math/BoundingBox.h>
#include <Utils/PrimitiveDefs.h>

#include <functional>
#include <vector>

// Where a scene's render objects changed, and when (by RenderScene::GetRevision), so something cached from part of
// the scene (e.g. a point light's shadow cube) can tell whether that part changed since it was made. It keeps at most
// k_capacity changes: past that, the two neighbouring changes whose merged box is smallest become one box around both,
// at the newer revision, so what changed is never lost, only told apart less finely. Smallest rather than oldest, so a
// box around much of the scene (e.g. from loading it) isn't kept alive by merging every new change into it.
class SceneChangeLog
{
public:
	// A couple of hundred objects moving in a frame (each logs where it was and where it is) before any are merged
	static constexpr size_t k_capacity = 256;

public:
	// Something within bounds changed at revision, which is never lower than the last change's. Empty bounds are
	// ignored.
	void Add(U64 revision, const BoundingBox& bounds);
	// Everything changed at revision (e.g. every object was removed): anything older is taken as changed
	void AddEverywhere(U64 revision);

	// Whether anything intersecting region changed after sinceRevision. True when the log doesn't go back that far.
	bool HasChangedWithin(const BoundingBox& region, U64 sinceRevision) const;
	// Calls visit with the bounds of each change after sinceRevision, newest first, so whatever cached part of the
	// scene can find what's out of date. False, without calling it, when the log doesn't go back that far (everything
	// changed since).
	bool ForEachChangeSince(U64 sinceRevision, const std::function<void(const BoundingBox&)>& visit) const;

private:
	// Changes at or before this revision are forgotten (AddEverywhere), so asking about anything older answers that it
	// changed
	void Forget(U64 revision);
	// Whether the log no longer tells what changed after sinceRevision
	bool HasForgottenSince(U64 sinceRevision) const;
	// Makes room for one more change: merges the neighbouring pair whose box around both has the least surface area
	void MergeClosestPair();
	// The surface area of the box around changes index and index + 1
	float CalculateMergedArea(size_t index) const;

private:
	struct Change
	{
		U64 Revision;
		BoundingBox Bounds;
	};

	// Oldest first
	std::vector<Change> m_changes;
	// By pair of neighbours, CalculateMergedArea(index), kept so a merge only works out the pairs it changed
	std::vector<float> m_mergedAreas;
	U64 m_forgottenRevision = 0;
};
