// Mesh instances (MeshBatches): the vertices in slot 0, and each instance's transform in slot 1, so a batch of
// instances is one draw. The depth prepass and forward pass draw with it, so both compute the same depths.
#include "Common/MeshVertex.hlsli"

// Mirrors GraphicsLayouts::ObjectBufferLayout, one an instance (MeshBatches::GetInputLayout). Each float4 is a column:
// glm stores matrices column by column, as the ObjectBuffer's registers read them. Semantics end in letters, not digits,
// as D3D11's input layout takes semantic index 0 for every element.
struct MeshInstanceInput
{
	float4 WorldA    : WORLDA;
	float4 WorldB    : WORLDB;
	float4 WorldC    : WORLDC;
	float4 WorldD    : WORLDD;
	float4 InvWorldA : INVWORLDA;
	float4 InvWorldB : INVWORLDB;
	float4 InvWorldC : INVWORLDC;
	float4 InvWorldD : INVWORLDD;
};

VertexToPixel VSMain(VertexInput vertex, MeshInstanceInput instance)
{
	// float4x4(a, b, c, d) takes rows, so the columns are transposed into place: the same matrix as the ObjectBuffer's
	const float4x4 world = transpose(float4x4(instance.WorldA, instance.WorldB, instance.WorldC, instance.WorldD));
	const float4x4 invWorld = transpose(float4x4(instance.InvWorldA, instance.InvWorldB, instance.InvWorldC, instance.InvWorldD));
	return TransformMeshVertex(vertex, world, invWorld);
}
