// Moves last frame's adapted log-luminance toward this frame's measurement (1x1 target).
// Output: r = adapted log-luminance, g = 1 once r holds a real measurement.
#include "Common/PostProcess.hlsli"

Texture2D LuminanceTexture : register(t0);
Texture2D PreviousAdapted  : register(t1);

float4 PSMain(PS_IN input) : SV_TARGET
{
	float2 weightedLog = LuminanceTexture.Load(int3(0, 0, (int)LuminanceMip)).rg;
	float2 previous = PreviousAdapted.Load(int3(0, 0, 0)).rg;

	// After a reset the history is meaningless; until something is measured, it stays invalid
	bool hasHistory = (Reset < 0.5f) && (previous.g > 0.5f);

	// Nothing but background on screen (e.g. meshes still streaming in): hold the current exposure
	if (weightedLog.g < 1e-4f)
	{
		return hasHistory ? float4(previous, 0.0f, 0.0f) : float4(0.0f, 0.0f, 0.0f, 0.0f);
	}

	float measured = weightedLog.r / weightedLog.g;

	// Frame-rate independent exponential smoothing, in log space so brightening and darkening feel symmetric
	float blend = hasHistory ? 1.0f - exp(-DeltaTime * AdaptationRate) : 1.0f;

	return float4(lerp(previous.r, measured, blend), 1.0f, 0.0f, 0.0f);
}
