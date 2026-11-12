// Material with some textures, but no normal map
#include "Common/ForwardTypes.hlsli"
#include "Common/Lighting.hlsli"

static const float DefaultSpecularIntensity = 0.5f; // Typical non-metal; see SurfaceData

float4 PSMain(VertexToPixel input) : SV_TARGET
{
	const float coverage = ApplyOpacityMask(input.UV);

	float3 vertexNormal = normalize(input.Normal);

	SurfaceData surface;
	surface.Albedo = GetAlbedo(input.UV, input.Tint);
	surface.VertexNormal = vertexNormal;
	surface.Normal = ApplyHeightMap(vertexNormal, input.WorldPos, input.UV);
	surface.SpecularIntensity = DefaultSpecularIntensity;
	surface.Shininess = input.Shininess;
	surface.WorldPos = input.WorldPos;
	surface.AmbientOcclusion = SampleAmbientOcclusion(input.Position.xy);

	float3 viewDir = normalize(input.CameraPos - input.WorldPos);
	float3 colour = ComputeLighting(surface, viewDir);

	// Linear and unclipped; PostProcessRenderStage applies exposure and tonemapping
	// Opacity is 1 unless the material is blended (MaterialInstance::BuildInstanceData); only the blended pass uses it.
	// Coverage is 1 unless a blended material has a mask.
	return float4(colour, input.Opacity * coverage);
}
