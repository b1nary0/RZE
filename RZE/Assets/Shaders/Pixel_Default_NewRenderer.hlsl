// Fallback for materials with no textures
#include "Common/ForwardTypes.hlsli"
#include "Common/Lighting.hlsli"

static const float3 DefaultAlbedo = float3(0.5f, 0.5f, 0.5f); // sRGB
static const float DefaultSpecularIntensity = 0.5f; // Typical non-metal; see SurfaceData

float4 PSMain(VertexToPixel input) : SV_TARGET
{
	SurfaceData surface;
	surface.Albedo = SRGBToLinear(DefaultAlbedo);
	surface.Normal = normalize(input.Normal);
	surface.VertexNormal = surface.Normal;
	surface.SpecularIntensity = DefaultSpecularIntensity;
	surface.Shininess = Shininess;
	surface.WorldPos = input.WorldPos;

	float3 viewDir = normalize(input.CameraPos - input.WorldPos);
	float3 colour = ComputeLighting(surface, viewDir);

	// Linear and unclipped; PostProcessRenderStage applies exposure and tonemapping
	return float4(colour, 1.0f);
}
