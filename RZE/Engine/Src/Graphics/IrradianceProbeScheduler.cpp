#include <StdAfx.h>
#include <Graphics/IrradianceProbeScheduler.h>

#include <Graphics/RenderScene.h>

#include <algorithm>
#include <cmath>
#include <numeric>

namespace
{
	// Probes within this many spacings of the camera along each axis are relit every frame
	constexpr int k_nearProbes = 2;
	// Probes within this many spacings of an object that changed are captured again
	constexpr float k_recaptureRadiusSpacings = 5.0f;

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
	if (!HaveBoundsMoved(m_fittedBounds, scene.GetBounds(), m_grid.Spacing))
	{
		return false;
	}

	m_fittedBounds = scene.GetBounds();
	m_grid = IrradianceProbeGrid::Fit(m_fittedBounds, targetProbeCount);

	const U32 probeCount = m_grid.GetProbeCount();
	m_captured.assign(probeCount, false);
	m_queued.assign(probeCount, true);
	m_relitFrame.assign(probeCount, k_neverFrame);
	m_captureQueue.resize(probeCount);
	std::iota(m_captureQueue.begin(), m_captureQueue.end(), 0u);
	SortCaptureQueue(cameraPosition);

	m_nextRelight = 0;
	m_checkedRevision = scene.GetRevision();
	return true;
}

void IrradianceProbeScheduler::QueueChangedProbes(const RenderScene& scene, const Vector3D& cameraPosition)
{
	if (scene.GetObjectsRevision() <= m_checkedRevision)
	{
		return;
	}

	const Vector3D reach(m_grid.Spacing * k_recaptureRadiusSpacings);
	bool hasQueued = false;
	for (U32 probe = 0; probe < m_grid.GetProbeCount(); ++probe)
	{
		if (!m_captured[probe] || m_queued[probe])
		{
			continue;
		}

		const Vector3D position = m_grid.GetProbePosition(probe);
		if (scene.HaveObjectsChangedWithin(BoundingBox(position - reach, position + reach), m_checkedRevision))
		{
			m_captureQueue.push_back(probe);
			m_queued[probe] = true;
			hasQueued = true;
		}
	}

	m_checkedRevision = scene.GetRevision();
	if (hasQueued)
	{
		SortCaptureQueue(cameraPosition);
	}
}

U32 IrradianceProbeScheduler::TakeCapture()
{
	AssertExpr(HasCaptureWaiting());

	const U32 probe = m_captureQueue.back();
	m_captureQueue.pop_back();
	m_queued[probe] = false;
	return probe;
}

void IrradianceProbeScheduler::MarkCaptured(U32 probe)
{
	m_captured[probe] = true;
}

void IrradianceProbeScheduler::ListRelights(U32 budget, const Vector3D& cameraPosition, U64 frameIndex, GraphicsLayouts::ProbeListLayout& list)
{
	for (U32 slot = 0; slot < list.Count; ++slot)
	{
		m_relitFrame[list.Probes[slot]] = frameIndex;
	}

	const U32 probeCount = m_grid.GetProbeCount();
	if (probeCount == 0)
	{
		return;
	}

	const U32 clampedBudget = std::max(std::min(budget, GraphicsLayouts::ProbeListLayout::k_maxProbes), list.Count);

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
				if (list.Count < clampedBudget && m_captured[probe] && m_relitFrame[probe] != frameIndex)
				{
					AddToRelight(probe, frameIndex, list);
				}
			}
		}
	}

	// Then the rest in turn
	for (U32 tried = 0; tried < probeCount && list.Count < clampedBudget; ++tried)
	{
		const U32 probe = m_nextRelight;
		m_nextRelight = (m_nextRelight + 1) % probeCount;
		if (m_captured[probe] && m_relitFrame[probe] != frameIndex)
		{
			AddToRelight(probe, frameIndex, list);
		}
	}
}

void IrradianceProbeScheduler::SortCaptureQueue(const Vector3D& cameraPosition)
{
	std::vector<float> distances(m_grid.GetProbeCount(), 0.0f);
	for (U32 probe : m_captureQueue)
	{
		distances[probe] = (m_grid.GetProbePosition(probe) - cameraPosition).Length();
	}
	std::sort(m_captureQueue.begin(), m_captureQueue.end(), [&distances](U32 a, U32 b) { return distances[a] > distances[b]; });
}

void IrradianceProbeScheduler::AddToRelight(U32 probe, U64 frameIndex, GraphicsLayouts::ProbeListLayout& list)
{
	list.Probes[list.Count++] = probe;
	m_relitFrame[probe] = frameIndex;
}
