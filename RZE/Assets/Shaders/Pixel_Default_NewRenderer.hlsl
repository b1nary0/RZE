// Fallback for materials with no textures
#include "Common/ForwardTypes.hlsli"
#include "Common/Lighting.hlsli"

static const float DefaultSpecularIntensity = 0.5f; // Typical non-metal; see SurfaceData

float4 PSMain(VertexToPixel input) : SV_TARGET
{
	SurfaceData surface;
	// Its tint, since it has no diffuse texture
	surface.Albedo = GetAlbedo(input.UV);
	surface.Normal = normalize(input.Normal);
	surface.VertexNormal = surface.Normal;
	surface.SpecularIntensity = DefaultSpecularIntensity;
	surface.Shininess = Shininess;
	surface.WorldPos = input.WorldPos;

	float3 viewDir = normalize(input.CameraPos - input.WorldPos);
	float3 colour = ComputeLighting(surface, viewDir);

	// Linear and unclipped; PostProcessRenderStage applies exposure and tonemapping
	// Opacity is 1 unless the material is blended (MaterialInstance::Create); only the blended pass uses it
	return float4(colour, Opacity);
}
