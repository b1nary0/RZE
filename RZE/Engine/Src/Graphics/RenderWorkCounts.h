#pragma once

// The scene work the render stages did that only happens when something in the scene changes (objects or lights
// moving), counted as it's recorded. RenderContext hands the stages RenderPipeline's, and RenderStats shows them per
// frame. Doubles, so RenderStats can hold their averages in the same struct.
struct RenderWorkCounts
{
	// IrradianceProbeStage
	double IrradianceProbesCaptured = 0.0;
	double IrradianceCaptureDraws = 0.0;
	double IrradianceProbesRelit = 0.0;
	// ReflectionProbeStage
	double ReflectionProbesCaptured = 0.0;
	double ReflectionCaptureDraws = 0.0;
	double ReflectionProbesRelit = 0.0;
	// ShadowRenderStage
	double SunShadowMapsRendered = 0.0;
	double PointShadowFacesRendered = 0.0;
	double ShadowDraws = 0.0;
};
