#ifndef POSTPROCESS_HLSLI
#define POSTPROCESS_HLSLI

#include "Common/ColourSpace.hlsli"

// Output of Vertex_RenderTargetQuad.hlsl
struct PS_IN
{
	float4 Position : SV_POSITION;
	float2 UV       : UV;
};

// Must match PostProcessRenderStage::ParamsLayout
cbuffer PostProcessParams : register(b0)
{
	float2 ViewportScale;        // Scene viewport size / scene target size; the viewport is the target's top-left corner
	float  DeltaTime;            // Seconds
	float  AdaptationRate;       // Per second; higher adapts faster
	float  MinExposure;
	float  MaxExposure;
	float  ExposureCompensation; // EV, applied after the clamp
	float  KeyValue;             // Linear value the scene's log-average luminance is exposed to
	float  LuminanceMip;         // The luminance texture's 1x1 mip
	float  Reset;                // > 0.5: jump straight to the measured luminance instead of adapting
	float  AutoExposure;         // > 0.5: expose to the adapted luminance; otherwise the exposure is 1
	// FXAA, from the view's anti-aliasing level (AntiAliasingSettings)
	float  EdgeThreshold;        // Less contrast than max(EdgeThresholdMin, this x the brightest neighbour's luma) isn't an edge
	float  EdgeThresholdMin;
	float  Subpixel;             // How much detail thinner than a pixel is softened, 0 to 1
	float  SearchStepCount;      // How many of SearchSteps the edge-end search takes
	float  _pad0;
	float4 SearchSteps[3];       // Each search step's distance along the edge, in pixels; 4 per element
};

#endif // POSTPROCESS_HLSLI
