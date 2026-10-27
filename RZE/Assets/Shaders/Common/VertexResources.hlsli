#ifndef VERTEX_RESOURCES_HLSLI
#define VERTEX_RESOURCES_HLSLI

// Mirrors RenderCamera (Graphics/RenderView.h)
cbuffer CameraBuffer : register(b0)
{
	float4x4 ViewProjection;
	float3 CameraPosition;
};

// Mirrors GraphicsLayouts::ObjectBufferLayout (Graphics/GraphicsLayouts.h)
cbuffer ObjectBuffer : register(b1)
{
	float4x4 World;
	float4x4 InvWorld;
};

#endif // VERTEX_RESOURCES_HLSLI
