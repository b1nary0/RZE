#pragma once

#include <Graphics/GraphicsLayouts.h>
#include <Graphics/IrradianceProbeGrid.h>
#include <Graphics/SettlingRegions.h>

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
//     objects within a few spacings of them change (more for bigger objects), once those have stayed still for a few
//     frames. Probes never captured and probes captured again take turns, so something changing near the camera
//     can't hold up the rest of the grid.
//   - Probes are relit only while their light is still changing: each relight bounces the light once more, so after
//     a change a probe owes a few relights (GlobalIlluminationSettings::SettleRelights), then none. A capture, or a
//     probe starting or stopping lighting anything (SetActive), changes the light everywhere, so every probe owes them;
//     a point light changing changes it around the light, so only the probes there do.
//   - Each frame's relighting lists the probes just captured, then those around lights that just changed (the nearest
//     the camera, if they're more than the budget holds), then those around the camera, then the rest in turn: only
//     those that owe relights and light something. Once none owes any, nothing is relit.
class IrradianceProbeScheduler
{
public:
	// Fits the grid to scene's Static bounds if they've moved by more than a spacing since it was last fitted (or it
	// never was). True if it did, so the probes' textures are made again: every probe is then waiting to be captured,
	// nearest cameraPosition first, and none is lit.
	bool FitGrid(const RenderScene& scene, U32 targetProbeCount, const Vector3D& cameraPosition);
	// Queues the captured probes within reach of Static render objects that changed (GetRecaptureReach of where each
	// was and where it is), once nothing within that reach has changed for settleFrames frames by frameIndex; or every
	// captured probe at once if the scene doesn't know where they changed. They keep their old captures until they're
	// captured again.
	void QueueChangedProbes(const RenderScene& scene, const Vector3D& cameraPosition, U64 frameIndex, U32 settleFrames);

	bool HasCaptureWaiting() const { return !m_fillQueue.empty() || !m_recaptureQueue.empty(); }
	// Whether probes are still waiting for their first capture since the grid was fitted
	bool IsFilling() const { return !m_fillQueue.empty(); }
	// The nearest probe waiting to be captured, taken off its queue: from the probes never captured and those to
	// capture again in turn while both are waiting. Only while HasCaptureWaiting.
	U32 TakeCapture();
	// Its capture is in place, so it can be relit
	void MarkCaptured(U32 probe);
	bool IsCaptured(U32 probe) const { return m_captured[probe]; }
	// What a capture of probe said, a few frames after it: whether it lights anything (it isn't inside geometry, and
	// sees a surface near it). An inactive probe isn't relit.
	void SetActive(U32 probe, bool isActive);
	// Until a capture says otherwise
	bool IsActive(U32 probe) const { return m_active[probe]; }

	// Has the captured, active probes around lights that changed how they light the scene since last time (within two
	// spacings of where each reached before and after) owe settleRelights relights each, and listed first next time,
	// the nearest cameraPosition if they're more than the budget holds; or every probe owe them if the change is everywhere (a directional light) or the
	// scene doesn't know where.
	void QueueChangedLights(const RenderScene& scene, const Vector3D& cameraPosition, U32 settleRelights);

	// Adds this frame's relights to list, after the probes already in it (those just captured): those around lights
	// that changed (QueueChangedLights), then those around cameraPosition, then the rest in turn, until it holds budget
	// (at most ProbeListLayout::k_maxProbes, at least what it already holds). Only captured, active probes that owe
	// relights, each at most once a frame; each listed pays one. Probes captured or turned on or off since last time
	// first have every probe owe settleRelights.
	void ListRelights(U32 budget, U32 settleRelights, const Vector3D& cameraPosition, U64 frameIndex, GraphicsLayouts::ProbeListLayout& list);
	// Whether any probe still owes a relight, so the probes' light is still changing
	bool IsSettling() const { return m_owingCount > 0 || m_hasChangedEverywhere; }

	const IrradianceProbeGrid& GetGrid() const { return m_grid; }

	// How far from a change in changedBounds probes are captured again, metres: twice its radius, so a bigger change
	// reaches further, within 2 to 5 spacings. Two spacings is as far as a probe's visibility map tells distances
	// apart; beyond that, a change only changes a probe's light by the part of its view it takes up.
	float GetRecaptureReach(const BoundingBox& changedBounds) const;

private:
	// A probe waiting to be captured or relit, and how far it was from the camera when it was queued
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
	// Lists probe, and pays one of the relights it owes
	void AddToRelight(U32 probe, U64 frameIndex, GraphicsLayouts::ProbeListLayout& list);
	// probe owes at least relights relights, if it's captured and active
	void OweRelights(U32 probe, U32 relights);
	void OweRelightsEverywhere(U32 relights);
	void ForgiveRelights(U32 probe);
	void ClearAroundChangedLights();

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
	// By probe: whether it lights anything
	std::vector<bool> m_active;
	// By probe: how many more times it's relit before its light settles; how many probes owe any
	std::vector<U8> m_owedRelights;
	U32 m_owingCount = 0;
	// Probes were captured, or turned on or off, since relights were last listed: every probe then owes them
	bool m_hasChangedEverywhere = false;
	// Around lights that changed since relights were last listed, in any order, each once; and by probe, whether it's
	// in there
	std::vector<QueuedProbe> m_aroundChangedLights;
	std::vector<bool> m_isAroundChangedLight;
	// The next probe to relight in turn
	U32 m_nextRelight = 0;
	// The scene's GetRevision when changed objects, and changed lights, were last looked for
	U64 m_checkedRevision = 0;
	U64 m_checkedLightRevision = 0;
	// Within reach of changes that haven't settled yet
	SettlingRegions m_unsettled;
};
