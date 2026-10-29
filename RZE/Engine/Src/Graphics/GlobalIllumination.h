#pragma once

#include <Utils/PrimitiveDefs.h>

// How much work the irradiance probes (IrradianceProbeStage), and reflection probes (ReflectionProbeStage), get. Engine.ini's [Rendering] sets them at start-up;
// RenderEngine owns them. Whether a view uses the probes at all is its own (RenderView::GlobalIllumination).
struct GlobalIlluminationSettings
{
	// About how many probes a scene's grid has, whatever its size (IrradianceProbeGrid::Fit)
	U32 TargetProbeCount = 4096;
	// Probes captured a frame, all in one batch (ProbeCapture), up to the 64 a batch holds
	U32 CapturesPerFrame = 64;
	// Probes relit a frame: those just captured and those near the camera first, then the rest in turn
	U32 RelightBudget = 512;
	// Frames a Static object must stay still before the irradiance and reflection probes around it are captured again,
	// so dragging one doesn't capture them every frame it moves (about 130 ms at 60 frames a second)
	U32 RecaptureSettleFrames = 8;
};
