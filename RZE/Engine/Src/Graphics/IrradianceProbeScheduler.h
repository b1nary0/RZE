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
//   - The grid is fitted to the scene's bounds, and fitted again when they move by more than a spacing. Then every
//     probe waits to be captured and none is lit.
//   - Probes wait to be captured, nearest the camera first, from when the grid is fitted, and again when render
//     objects within a few spacings of them change.
//   - Each frame's relighting lists the probes just captured, then those around the camera, then the rest in turn.
class IrradianceProbeScheduler
{
public:
	// Fits the grid to scene's bounds if they've moved by more than a spacing since it was last fitted (or it never
	// was). True if it did, so the probes' textures are made again: every probe is then waiting to be captured,
	// nearest cameraPosition first, and none is lit.
	bool FitGrid(const RenderScene& scene, U32 targetProbeCount, const Vector3D& cameraPosition);
	// Queues the captured probes within a few spacings of render objects that changed since last time, nearest
	// cameraPosition first. They keep their old captures until they're captured again.
	void QueueChangedProbes(const RenderScene& scene, const Vector3D& cameraPosition);

	bool HasCaptureWaiting() const { return !m_captureQueue.empty(); }
	// The nearest probe waiting to be captured, taken off the queue. Only while HasCaptureWaiting.
	U32 TakeCapture();
	// Its capture is in place, so it can be relit
	void MarkCaptured(U32 probe);
	bool IsCaptured(U32 probe) const { return m_captured[probe]; }

	// Adds this frame's relights to list, after the probes already in it (those just captured): those around
	// cameraPosition, then the rest in turn, until it holds budget (at most ProbeListLayout::k_maxProbes, at least
	// what it already holds). Only captured probes, each at most once a frame.
	void ListRelights(U32 budget, const Vector3D& cameraPosition, U64 frameIndex, GraphicsLayouts::ProbeListLayout& list);

	const IrradianceProbeGrid& GetGrid() const { return m_grid; }

private:
	// Sorts the capture queue so the nearest to cameraPosition is at the back
	void SortCaptureQueue(const Vector3D& cameraPosition);
	void AddToRelight(U32 probe, U64 frameIndex, GraphicsLayouts::ProbeListLayout& list);

private:
	IrradianceProbeGrid m_grid;
	// What m_grid was fitted to
	BoundingBox m_fittedBounds;

	// Probes waiting to be captured, farthest from the camera first, so the nearest is taken off the back
	std::vector<U32> m_captureQueue;
	// By probe: whether it has a capture, whether it's in m_captureQueue, and the frame it was last relit
	std::vector<bool> m_captured;
	std::vector<bool> m_queued;
	std::vector<U64> m_relitFrame;
	// The next probe to relight in turn
	U32 m_nextRelight = 0;
	// The scene's GetRevision when changed objects were last looked for
	U64 m_checkedRevision = 0;
};
