#pragma once

#include <Utils/PrimitiveDefs.h>

// How much work the irradiance probes (IrradianceProbeStage) get. Engine.ini's [Rendering] sets them at start-up;
// RenderEngine owns them. Whether a view uses the probes at all is its own (RenderView::GlobalIllumination).
struct GlobalIlluminationSettings
{
	// About how many probes a scene's grid has, whatever its size (IrradianceProbeGrid::Fit)
	U32 TargetProbeCount = 4096;
	// Draws a frame for capturing probes; at about 11 draws a face in Sponza, 4000 is about 60 probes
	U32 CaptureDrawBudget = 4000;
	// Probes relit a frame: those just captured and those near the camera first, then the rest in turn
	U32 RelightBudget = 512;
};
