#include <Graphics/IrradianceProbeScheduler.h>

#include <Graphics/RenderScene.h>

#include <algorithm>
#include <cmath>

namespace
{
	// Probes within this many spacings of the camera along each axis are relit first, of those owing relights
	constexpr int k_nearProbes = 2;
	// How far past a changed light's reach probes are relit, in spacings: those beyond read only probes within it
	constexpr float k_lightReachSpacings = 2.0f;
	// How far probes are captured again around a change: this many times its radius, within these many spacings
	constexpr float k_recaptureReachPerRadius = 2.0f;
	constexpr float k_minRecaptureReachSpacings = 2.0f;
	constexpr float k_maxRecaptureReachSpacings = 5.0f;

	constexpr U64 k_neverFrame = ~0ull;

	// Whether either corner moved by more than tolerance along any axis, or one is empty and the other isn't
	bool HaveBoundsMoved(const BoundingBox& from, const BoundingBox& to, float tolerance)
	{
		if (from.IsEmpty() || to.IsEmpty())
		{
			return from.IsEmpty() != to.IsEmpty();
		}

		for (int axis = 0; axis < 3; ++axis)
		{
			if (std::abs(from.GetMin()[axis] - to.GetMin()[axis]) > tolerance || std::abs(from.GetMax()[axis] - to.GetMax()[axis]) > tolerance)
			{
				return true;
			}
		}
		return false;
	}
}

bool IrradianceProbeScheduler::FitGrid(const RenderScene& scene, U32 targetProbeCount, const Vector3D& cameraPosition)
{
	// Static objects alone, so what moves doesn't fit the grid again
	if (!HaveBoundsMoved(m_fittedBounds, scene.GetStaticBounds(), m_grid.Spacing))
	{
		return false;
	}

	m_fittedBounds = scene.GetStaticBounds();
	m_grid = IrradianceProbeGrid::Fit(m_fittedBounds, targetProbeCount);

	const U32 probeCount = m_grid.GetProbeCount();
	m_captured.assign(probeCount, false);
	m_queued.assign(probeCount, true);
	m_relitFrame.assign(probeCount, k_neverFrame);
	m_active.assign(probeCount, true);
	m_owedRelights.assign(probeCount, 0);
	m_owingCount = 0;
	m_hasChangedEverywhere = false;
	m_aroundChangedLights.clear();
	m_isAroundChangedLight.assign(probeCount, false);
	m_recaptureQueue.clear();
	m_fillQueue.resize(probeCount);
	for (U32 probe = 0; probe < probeCount; ++probe)
	{
		m_fillQueue[probe] = { (m_grid.GetProbePosition(probe) - cameraPosition).Length(), probe };
	}
	std::make_heap(m_fillQueue.begin(), m_fillQueue.end(), &IsFurther);

	m_nextRelight = 0;
	m_checkedRevision = scene.GetRevision();
	m_checkedLightRevision = scene.GetRevision();
	m_unsettled.Clear();
	return true;
}

void IrradianceProbeScheduler::QueueChangedProbes(const RenderScene& scene, const Vector3D& cameraPosition, U64 frameIndex, U32 settleFrames)
{
	const auto queueWithin = [this, &cameraPosition](const BoundingBox& region)
	{
		const IrradianceProbeGrid::ProbeRange range = m_grid.GetProbesWithin(region);
		if (range.IsEmpty)
		{
			return;
		}

		for (U32 z = range.Min[2]; z <= range.Max[2]; ++z)
		{
			for (U32 y = range.Min[1]; y <= range.Max[1]; ++y)
			{
				for (U32 x = range.Min[0]; x <= range.Max[0]; ++x)
				{
					const U32 probe = x + m_grid.CountX * (y + m_grid.CountY * z);
					if (m_captured[probe] && !m_queued[probe])
					{
						QueueRecapture(probe, cameraPosition);
					}
				}
			}
		}
	};

	// Movable objects aren't captured, so only Static ones change what the probes see
	if (scene.GetStaticObjectsRevision() > m_checkedRevision)
	{
		const auto touchReach = [this, frameIndex](const BoundingBox& changedBounds)
		{
			const Vector3D reach(GetRecaptureReach(changedBounds));
			m_unsettled.Touch(BoundingBox(changedBounds.GetMin() - reach, changedBounds.GetMax() + reach), frameIndex);
		};

		// Where isn't known: everything that's been captured, at once
		if (!scene.GetStaticChanges().ForEachChangeSince(m_checkedRevision, touchReach))
		{
			m_unsettled.Clear();
			for (U32 probe = 0; probe < m_grid.GetProbeCount(); ++probe)
			{
				if (m_captured[probe] && !m_queued[probe])
				{
					QueueRecapture(probe, cameraPosition);
				}
			}
		}
		m_checkedRevision = scene.GetRevision();
	}

	m_unsettled.TakeSettled(frameIndex, settleFrames, queueWithin);
}

U32 IrradianceProbeScheduler::TakeCapture()
{
	AssertExpr(HasCaptureWaiting());

	// In turn while both are waiting
	const bool takesRecapture = m_fillQueue.empty() || (!m_recaptureQueue.empty() && m_takesRecaptureNext);
	m_takesRecaptureNext = !takesRecapture;

	const U32 probe = PopNearest(takesRecapture ? m_recaptureQueue : m_fillQueue);
	m_queued[probe] = false;
	return probe;
}

void IrradianceProbeScheduler::MarkCaptured(U32 probe)
{
	m_captured[probe] = true;
	m_hasChangedEverywhere = true;
}

void IrradianceProbeScheduler::SetActive(U32 probe, bool isActive)
{
	if (m_active[probe] == isActive)
	{
		return;
	}

	m_active[probe] = isActive;
	if (!isActive)
	{
		ForgiveRelights(probe);
	}
	// What reads it gains or loses its light
	m_hasChangedEverywhere = true;
}

void IrradianceProbeScheduler::QueueChangedLights(const RenderScene& scene, const Vector3D& cameraPosition, U32 settleRelights)
{
	if (m_grid.IsEmpty())
	{
		return;
	}

	const Vector3D reach(m_grid.Spacing * k_lightReachSpacings);
	const bool isKnown = scene.GetLightChanges().ForEachChangeSince(m_checkedLightRevision, [this, &reach, &cameraPosition, settleRelights](const BoundingBox& changed)
	{
		const IrradianceProbeGrid::ProbeRange range = m_grid.GetProbesWithin(BoundingBox(changed.GetMin() - reach, changed.GetMax() + reach));
		if (range.IsEmpty)
		{
			return;
		}

		// The camera's place in grid units, so each probe's distance is a few multiplies (this runs for every probe
		// around a flickering light, every frame)
		const Vector3D fromOrigin = (cameraPosition - m_grid.Origin) / m_grid.Spacing;
		const float cameraCell[3] = { fromOrigin.X(), fromOrigin.Y(), fromOrigin.Z() };
		for (U32 z = range.Min[2]; z <= range.Max[2]; ++z)
		{
			const float dz = static_cast<float>(z) - cameraCell[2];
			for (U32 y = range.Min[1]; y <= range.Max[1]; ++y)
			{
				const float dy = static_cast<float>(y) - cameraCell[1];
				for (U32 x = range.Min[0]; x <= range.Max[0]; ++x)
				{
					const U32 probe = x + m_grid.CountX * (y + m_grid.CountY * z);
					if (m_captured[probe] && m_active[probe] && !m_isAroundChangedLight[probe])
					{
						m_isAroundChangedLight[probe] = true;
						const float dx = static_cast<float>(x) - cameraCell[0];
						OweRelights(probe, settleRelights);
						m_aroundChangedLights.push_back({ std::sqrt(dx * dx + dy * dy + dz * dz) * m_grid.Spacing, probe });
					}
				}
			}
		}
	});
	m_checkedLightRevision = scene.GetRevision();

	// Everywhere: in the usual order
	if (!isKnown)
	{
		OweRelightsEverywhere(settleRelights);
		ClearAroundChangedLights();
	}
}

void IrradianceProbeScheduler::ListRelights(U32 budget, U32 settleRelights, const Vector3D& cameraPosition, U64 frameIndex, GraphicsLayouts::ProbeListLayout& list)
{
	if (m_hasChangedEverywhere)
	{
		m_hasChangedEverywhere = false;
		OweRelightsEverywhere(settleRelights);
	}

	// Those already listed: just captured
	const U32 alreadyListed = list.Count;
	list.Count = 0;
	for (U32 slot = 0; slot < alreadyListed; ++slot)
	{
		AddToRelight(list.Probes[slot], frameIndex, list);
	}

	const U32 probeCount = m_grid.GetProbeCount();
	const U32 clampedBudget = std::max(std::min(budget, GraphicsLayouts::ProbeListLayout::k_maxProbes), list.Count);
	const auto isWaiting = [this, frameIndex](U32 probe)
	{
		return m_owedRelights[probe] > 0 && m_captured[probe] && m_active[probe] && m_relitFrame[probe] != frameIndex;
	};

	// Those around lights that changed, the nearest the camera if there are more than the budget holds (those it
	// doesn't reach still owe relights). Unsorted otherwise.
	const size_t room = clampedBudget - list.Count;
	if (m_aroundChangedLights.size() > room)
	{
		std::nth_element(m_aroundChangedLights.begin(), m_aroundChangedLights.begin() + static_cast<std::ptrdiff_t>(room), m_aroundChangedLights.end(),
			[](const QueuedProbe& a, const QueuedProbe& b) { return a.Distance < b.Distance; });
	}
	for (const QueuedProbe& queued : m_aroundChangedLights)
	{
		if (list.Count < clampedBudget && isWaiting(queued.Probe))
		{
			AddToRelight(queued.Probe, frameIndex, list);
		}
	}
	ClearAroundChangedLights();

	if (probeCount == 0 || m_owingCount == 0)
	{
		return;
	}

	// Those around the camera
	const Vector3D cameraCell = (cameraPosition - m_grid.Origin) / m_grid.Spacing;
	const int counts[3] = { static_cast<int>(m_grid.CountX), static_cast<int>(m_grid.CountY), static_cast<int>(m_grid.CountZ) };
	int nearest[3];
	for (int axis = 0; axis < 3; ++axis)
	{
		nearest[axis] = std::clamp(static_cast<int>(std::lround(cameraCell[axis])), 0, counts[axis] - 1);
	}
	for (int z = std::max(nearest[2] - k_nearProbes, 0); z <= std::min(nearest[2] + k_nearProbes, counts[2] - 1); ++z)
	{
		for (int y = std::max(nearest[1] - k_nearProbes, 0); y <= std::min(nearest[1] + k_nearProbes, counts[1] - 1); ++y)
		{
			for (int x = std::max(nearest[0] - k_nearProbes, 0); x <= std::min(nearest[0] + k_nearProbes, counts[0] - 1); ++x)
			{
				const U32 probe = static_cast<U32>(x + counts[0] * (y + counts[1] * z));
				if (list.Count < clampedBudget && isWaiting(probe))
				{
					AddToRelight(probe, frameIndex, list);
				}
			}
		}
	}

	// Then the rest in turn
	for (U32 tried = 0; tried < probeCount && list.Count < clampedBudget && m_owingCount > 0; ++tried)
	{
		const U32 probe = m_nextRelight;
		m_nextRelight = (m_nextRelight + 1) % probeCount;
		if (isWaiting(probe))
		{
			AddToRelight(probe, frameIndex, list);
		}
	}
}

float IrradianceProbeScheduler::GetRecaptureReach(const BoundingBox& changedBounds) const
{
	return std::clamp(changedBounds.GetRadius() * k_recaptureReachPerRadius,
		m_grid.Spacing * k_minRecaptureReachSpacings, m_grid.Spacing * k_maxRecaptureReachSpacings);
}

U32 IrradianceProbeScheduler::PopNearest(CaptureQueue& queue)
{
	std::pop_heap(queue.begin(), queue.end(), &IsFurther);
	const U32 probe = queue.back().Probe;
	queue.pop_back();
	return probe;
}

void IrradianceProbeScheduler::QueueRecapture(U32 probe, const Vector3D& cameraPosition)
{
	m_recaptureQueue.push_back({ (m_grid.GetProbePosition(probe) - cameraPosition).Length(), probe });
	std::push_heap(m_recaptureQueue.begin(), m_recaptureQueue.end(), &IsFurther);
	m_queued[probe] = true;
}

void IrradianceProbeScheduler::AddToRelight(U32 probe, U64 frameIndex, GraphicsLayouts::ProbeListLayout& list)
{
	list.Probes[list.Count++] = probe;
	m_relitFrame[probe] = frameIndex;
	if (m_owedRelights[probe] > 0 && --m_owedRelights[probe] == 0)
	{
		--m_owingCount;
	}
}

void IrradianceProbeScheduler::OweRelights(U32 probe, U32 relights)
{
	if (!m_captured[probe] || !m_active[probe] || relights == 0)
	{
		return;
	}

	if (m_owedRelights[probe] == 0)
	{
		++m_owingCount;
	}
	const U8 owed = static_cast<U8>(std::min(relights, 255u));
	m_owedRelights[probe] = std::max(m_owedRelights[probe], owed);
}

void IrradianceProbeScheduler::OweRelightsEverywhere(U32 relights)
{
	for (U32 probe = 0; probe < m_grid.GetProbeCount(); ++probe)
	{
		OweRelights(probe, relights);
	}
}

void IrradianceProbeScheduler::ClearAroundChangedLights()
{
	for (const QueuedProbe& queued : m_aroundChangedLights)
	{
		m_isAroundChangedLight[queued.Probe] = false;
	}
	m_aroundChangedLights.clear();
}

void IrradianceProbeScheduler::ForgiveRelights(U32 probe)
{
	if (m_owedRelights[probe] > 0)
	{
		m_owedRelights[probe] = 0;
		--m_owingCount;
	}
}
