#include "Common/ForwardTypes.hlsli"
#include "Common/VertexResources.hlsli"

VertexToPixel VSMain(VertexInput input)
{
	VertexToPixel output;

	float4 worldPos = mul(World, float4(input.Position, 1.0f));

	output.Position = mul(ViewProjection, worldPos);
	output.WorldPos = worldPos.xyz;
	// Normals transform by the inverse-transpose so non-uniform scale doesn't skew them
	output.Normal = normalize(mul(input.Normal, (float3x3)InvWorld));
	output.Tangent = normalize(mul((float3x3)World, input.Tangent));
	output.UV = input.UV;
	output.CameraPos = CameraPosition;

	return output;
}
