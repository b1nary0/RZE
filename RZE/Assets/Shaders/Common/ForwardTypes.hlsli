#ifndef FORWARD_TYPES_HLSLI
#define FORWARD_TYPES_HLSLI

// Vertex layout as described by ForwardRenderStage's ShaderInputLayout
struct VertexInput
{
	float3 Position : POSITION;
	float3 Normal   : NORMAL;
	float2 UV       : UV;
	float3 Tangent  : TANGENT;
};

// Shared VS output / PS input for the forward pass. All vectors are world space.
struct VertexToPixel
{
	float4 Position  : SV_POSITION;
	float3 Normal    : NORMAL;
	float2 UV        : UV;
	float3 Tangent   : TANGENT;
	float3 WorldPos  : POSITION;
	float3 CameraPos : POSITION1;
};

#endif // FORWARD_TYPES_HLSLI
