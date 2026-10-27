#include <StdAfx.h>
#include <Graphics/SettlingRegions.h>

#include <algorithm>

void SettlingRegions::Touch(const BoundingBox& box, U64 frame)
{
	if (box.IsEmpty())
	{
		return;
	}

	// Grown by every box it overlaps, which may then overlap more, until none is left that it does
	Region touched = { box, frame };
	bool hasMerged = true;
	while (hasMerged)
	{
		hasMerged = false;
		for (auto iter = m_regions.begin(); iter != m_regions.end(); ++iter)
		{
			if (iter->Box.Intersects(touched.Box))
			{
				touched.Box.Grow(iter->Box);
				m_regions.erase(iter);
				hasMerged = true;
				break;
			}
		}
	}
	m_regions.push_back(touched);
}

void SettlingRegions::TakeSettled(U64 frame, U64 settleFrames, const std::function<void(const BoundingBox&)>& settle)
{
	const auto isSettled = [frame, settleFrames](const Region& region) { return frame - region.ChangedFrame >= settleFrames; };
	for (const Region& region : m_regions)
	{
		if (isSettled(region))
		{
			settle(region.Box);
		}
	}
	m_regions.erase(std::remove_if(m_regions.begin(), m_regions.end(), isSettled), m_regions.end());
}
