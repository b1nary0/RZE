// Downsamples the scene viewport into the luminance texture's mip 0. Its mip chain then
// averages down to the scene's weighted log-average luminance.
#include "Common/PostProcess.hlsli"

Texture2D SceneTexture : register(t0);

float4 PSMain(PS_IN input) : SV_TARGET
{
	float2 sceneSize;
	SceneTexture.GetDimensions(sceneSize.x, sceneSize.y);

	// Point load: bilinear filtering would blend geometry with the alpha 0 background at silhouettes
	int2 texel = int2(input.UV * ViewportScale * sceneSize);
	float4 scene = SceneTexture.Load(int3(texel, 0));

	// Background pixels (alpha 0) carry no weight, so empty sky doesn't drag exposure up
	float weight = scene.a;
	float logLuminance = log(max(Luminance(scene.rgb), 1e-4f));

	return float4(weight * logLuminance, weight, 0.0f, 0.0f);
}
