#pragma once

#include <Utils/Math/BoundingBox.h>
#include <Utils/PrimitiveDefs.h>

#include <functional>
#include <vector>

// Where things changed, each box kept until nothing has changed in it for a while, so work a change causes (e.g.
// capturing probes again) waits until what's moving stops rather than redoing it every frame it moves. Changes that
// overlap share one box, which settles once they've all stopped.
class SettlingRegions
{
public:
	// Something changed within box at frame: it joins every box it overlaps, which then all count as changed at frame,
	// or starts one of its own. Empty boxes are ignored.
	void Touch(const BoundingBox& box, U64 frame);
	// Calls settle with each box nothing has changed in for settleFrames frames or more by frame, and forgets them
	void TakeSettled(U64 frame, U64 settleFrames, const std::function<void(const BoundingBox&)>& settle);

	bool IsEmpty() const { return m_regions.empty(); }
	void Clear() { m_regions.clear(); }

private:
	struct Region
	{
		BoundingBox Box;
		U64 ChangedFrame;
	};

	std::vector<Region> m_regions;
};
