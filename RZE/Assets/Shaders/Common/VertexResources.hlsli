#ifndef VERTEX_RESOURCES_HLSLI
#define VERTEX_RESOURCES_HLSLI

// Mirrors RenderCamera (Graphics/RenderView.h)
cbuffer CameraBuffer : register(b0)
{
	float4x4 ViewProjection;
	float3 CameraPosition;
};

#endif // VERTEX_RESOURCES_HLSLI
