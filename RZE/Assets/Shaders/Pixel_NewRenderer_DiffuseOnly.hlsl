// Material with only a diffuse map
#include "Common/ForwardTypes.hlsli"
#include "Common/Lighting.hlsli"

static const float DefaultSpecularIntensity = 0.5f; // Typical non-metal; see SurfaceData

float4 PSMain(VertexToPixel input) : SV_TARGET
{
	ApplyOpacityMask(input.UV);

	float4 diffuseSample = DiffuseMap.Sample(LinearSampler, input.UV);

	float3 vertexNormal = normalize(input.Normal);

	SurfaceData surface;
	surface.Albedo = SRGBToLinear(diffuseSample.rgb);
	surface.VertexNormal = vertexNormal;
	surface.Normal = ApplyHeightMap(vertexNormal, input.WorldPos, input.UV);
	surface.SpecularIntensity = DefaultSpecularIntensity;
	surface.Shininess = Shininess;
	surface.WorldPos = input.WorldPos;

	float3 viewDir = normalize(input.CameraPos - input.WorldPos);
	float3 colour = ComputeLighting(surface, viewDir);

	// Linear and unclipped; PostProcessRenderStage applies exposure and tonemapping
	return float4(colour, 1.0f);
}
