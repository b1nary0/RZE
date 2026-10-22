// Material with a normal map, and diffuse and specular maps if it has them
#include "Common/ForwardTypes.hlsli"
#include "Common/Lighting.hlsli"

static const float DefaultSpecularIntensity = 0.5f; // Typical non-metal; see SurfaceData

float4 PSMain(VertexToPixel input) : SV_TARGET
{
	const float coverage = ApplyOpacityMask(input.UV);

	float specularSample = HasSpecularMap > 0.5f ? SpecularMap.Sample(LinearSampler, input.UV).r : DefaultSpecularIntensity;
	float3 normalSample = NormalMap.Sample(LinearSampler, input.UV).rgb;

	float3 vertexNormal = normalize(input.Normal);

	SurfaceData surface;
	surface.Albedo = GetAlbedo(input.UV);
	surface.VertexNormal = vertexNormal;
	surface.Normal = PerturbNormal(vertexNormal, normalize(input.Tangent), normalSample);
	surface.Normal = ApplyHeightMap(surface.Normal, input.WorldPos, input.UV);
	surface.SpecularIntensity = specularSample;
	surface.Shininess = Shininess;
	surface.WorldPos = input.WorldPos;

	float3 viewDir = normalize(input.CameraPos - input.WorldPos);
	float3 colour = ComputeLighting(surface, viewDir);

	// Linear and unclipped; PostProcessRenderStage applies exposure and tonemapping
	// Opacity is 1 unless the material is blended (MaterialInstance::Create); only the blended pass uses it.
	// Coverage is 1 unless a blended material has a mask.
	return float4(colour, Opacity * coverage);
}
