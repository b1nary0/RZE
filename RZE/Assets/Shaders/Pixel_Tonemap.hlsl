// Linear scene colour -> exposed, tonemapped, sRGB-encoded colour for the 8-bit render target
#include "Common/PostProcess.hlsli"

Texture2D SceneTexture     : register(t0);
Texture2D AdaptedLuminance : register(t1);

// sRGB; shown as-is where nothing was drawn
static const float3 BackgroundColour = float3(0.25f, 0.25f, 0.35f);

float4 PSMain(PS_IN input) : SV_TARGET
{
	// The viewport matches the scene's, so pixel positions line up 1:1
	float4 scene = SceneTexture.Load(int3(input.Position.xy, 0));

	if (scene.a <= 0.0f)
	{
		return float4(BackgroundColour, 1.0f);
	}

	float averageLuminance = exp(AdaptedLuminance.Load(int3(0, 0, 0)).r);
	float exposure = clamp(KeyValue / averageLuminance, MinExposure, MaxExposure) * exp2(ExposureCompensation);

	return float4(LinearToSRGB(TonemapACES(scene.rgb * exposure)), 1.0f);
}
