// Material with diffuse, specular and normal maps
#include "Common/ForwardTypes.hlsli"
#include "Common/Lighting.hlsli"

float4 PSMain(VertexToPixel input) : SV_TARGET
{
	const float coverage = ApplyOpacityMask(input.UV);

	float4 diffuseSample = DiffuseMap.Sample(LinearSampler, input.UV);
	float specularSample = SpecularMap.Sample(LinearSampler, input.UV).r;
	float3 normalSample = NormalMap.Sample(LinearSampler, input.UV).rgb;

	float3 vertexNormal = normalize(input.Normal);

	SurfaceData surface;
	surface.Albedo = SRGBToLinear(diffuseSample.rgb);
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
