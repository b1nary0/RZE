#pragma once

#include <Graphics/GraphicsLayouts.h>
#include <Graphics/IrradianceProbeGrid.h>

#include <Utils/Math/BoundingBox.h>
#include <Utils/Math/Vector3D.h>
#include <Utils/PrimitiveDefs.h>

#include <vector>

class RenderScene;

// Which of a scene's irradiance probes IrradianceProbeStage captures and relights each frame, and the grid they're on.
// CPU only, so it can be tested without a renderer; the stage does the GPU work it picks.
//   - The grid is fitted to the bounds of the scene's Static objects, and fitted again when they move by more than a
//     spacing. Then every probe waits to be captured and none is lit. Movable objects aren't captured
//     (ERenderMobility), so they change nothing here.
//   - Probes wait to be captured, nearest the camera first, from when the grid is fitted, and again when render
//     objects within a few spacings of them change (more for bigger objects). Probes never captured and probes
//     captured again take turns, so something changing near the camera can't hold up the rest of the grid.
//   - Each frame's relighting lists the probes just captured, then those around the camera, then the rest in turn.
class IrradianceProbeScheduler
{
public:
	// Fits the grid to scene's Static bounds if they've moved by more than a spacing since it was last fitted (or it
	// never was). True if it did, so the probes' textures are made again: every probe is then waiting to be captured,
	// nearest cameraPosition first, and none is lit.
	bool FitGrid(const RenderScene& scene, U32 targetProbeCount, const Vector3D& cameraPosition);
	// Queues the captured probes within reach of Static render objects that changed since last time (GetRecaptureReach of
	// where each was and where it is), or every captured probe if the scene doesn't know where they changed. They keep
	// their old captures until they're captured again.
	void QueueChangedProbes(const RenderScene& scene, const Vector3D& cameraPosition);

	bool HasCaptureWaiting() const { return !m_fillQueue.empty() || !m_recaptureQueue.empty(); }
	// Whether probes are still waiting for their first capture since the grid was fitted
	bool IsFilling() const { return !m_fillQueue.empty(); }
	// The nearest probe waiting to be captured, taken off its queue: from the probes never captured and those to
	// capture again in turn while both are waiting. Only while HasCaptureWaiting.
	U32 TakeCapture();
	// Its capture is in place, so it can be relit
	void MarkCaptured(U32 probe);
	bool IsCaptured(U32 probe) const { return m_captured[probe]; }

	// Adds this frame's relights to list, after the probes already in it (those just captured): those around
	// cameraPosition, then the rest in turn, until it holds budget (at most ProbeListLayout::k_maxProbes, at least
	// what it already holds). Only captured probes, each at most once a frame.
	void ListRelights(U32 budget, const Vector3D& cameraPosition, U64 frameIndex, GraphicsLayouts::ProbeListLayout& list);

	const IrradianceProbeGrid& GetGrid() const { return m_grid; }

	// How far from a change in changedBounds probes are captured again, metres: twice its radius, so a bigger change
	// reaches further, within 2 to 5 spacings. Two spacings is as far as a probe's visibility map tells distances
	// apart; beyond that, a change only changes a probe's light by the part of its view it takes up.
	float GetRecaptureReach(const BoundingBox& changedBounds) const;

private:
	// A probe waiting to be captured, and how far it was from the camera when it was queued
	struct QueuedProbe
	{
		float Distance;
		U32 Probe;
	};
	// Min-heaps by distance (std::push_heap and std::pop_heap with IsFurther), so the nearest is at the front
	using CaptureQueue = std::vector<QueuedProbe>;

	static bool IsFurther(const QueuedProbe& a, const QueuedProbe& b) { return a.Distance > b.Distance; }
	static U32 PopNearest(CaptureQueue& queue);

	// Queues probe, which mustn't be queued already, to be captured again
	void QueueRecapture(U32 probe, const Vector3D& cameraPosition);
	void AddToRelight(U32 probe, U64 frameIndex, GraphicsLayouts::ProbeListLayout& list);

private:
	IrradianceProbeGrid m_grid;
	// What m_grid was fitted to
	BoundingBox m_fittedBounds;

	// Probes waiting to be captured: those never captured since the grid was fitted, and those to capture again
	CaptureQueue m_fillQueue;
	CaptureQueue m_recaptureQueue;
	// Whether TakeCapture takes from m_recaptureQueue next, while both are waiting
	bool m_takesRecaptureNext = false;
	// By probe: whether it has a capture, whether it's in a capture queue, and the frame it was last relit
	std::vector<bool> m_captured;
	std::vector<bool> m_queued;
	std::vector<U64> m_relitFrame;
	// The next probe to relight in turn
	U32 m_nextRelight = 0;
	// The scene's GetRevision when changed objects were last looked for
	U64 m_checkedRevision = 0;
};
