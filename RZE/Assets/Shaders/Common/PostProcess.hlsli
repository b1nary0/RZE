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
	float2 _pad0;
};

#endif // POSTPROCESS_HLSLI
