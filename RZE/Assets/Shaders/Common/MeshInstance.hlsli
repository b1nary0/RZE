#ifndef MESH_INSTANCE_HLSLI
#define MESH_INSTANCE_HLSLI

// A draw of a mesh as its shaders read it, besides its geometry and what its material binds (MeshInstanceFormat):
// from an instance stream, one an instance, or from a constant buffer, one a draw (ProbeCapture's CaptureDraw).

// Mirrors GraphicsLayouts::MeshInstanceLayout: where the draw is, and the material values it carries. As a constant
// buffer holds it; an instance stream's MeshInstanceInput is read into one (ReadMeshInstance).
struct MeshInstance
{
	float4x4 World;
	float4x4 InvWorld;
	// InstanceMaterialLayout's registers (UnpackInstanceMaterial)
	float4 MaterialA;
	float4 MaterialB;
};

// A MeshInstance as MeshInstanceFormats::Forward streams it, in its input layout's order. Each float4 of a matrix is
// a column: glm stores matrices column by column, as a constant buffer's registers read them. Semantics end in
// letters, not digits, as D3D11's input layout takes semantic index 0 for every element.
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
	float4 MaterialA : MATERIALA;
	float4 MaterialB : MATERIALB;
};

// A draw's world transform as MeshInstanceFormats::Depth streams it (GraphicsLayouts::MeshDepthInstanceLayout): its
// four columns' first three values, a MeshWorld
struct MeshDepthInstanceInput
{
	float3 WorldA : WORLDA;
	float3 WorldB : WORLDB;
	float3 WorldC : WORLDC;
	float3 WorldD : WORLDD;
};

MeshInstance ReadMeshInstance(MeshInstanceInput input)
{
	MeshInstance instance;
	// float4x4(a, b, c, d) takes rows, so the columns are transposed into place: the same matrix a constant buffer gives
	instance.World = transpose(float4x4(input.WorldA, input.WorldB, input.WorldC, input.WorldD));
	instance.InvWorld = transpose(float4x4(input.InvWorldA, input.InvWorldB, input.InvWorldC, input.InvWorldD));
	instance.MaterialA = input.MaterialA;
	instance.MaterialB = input.MaterialB;
	return instance;
}

#endif // MESH_INSTANCE_HLSLI
