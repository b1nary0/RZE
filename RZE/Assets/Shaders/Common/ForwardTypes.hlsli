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

// The material values a draw carries itself, the same across it, rather than reading from the MaterialBuffer: draws
// that differ only in these bind the same things, so they share a batch. Unpacked from
// GraphicsLayouts::InstanceMaterialLayout's two registers (UnpackInstanceMaterial).
struct InstanceMaterial
{
	float3 Tint; // sRGB; multiplies DiffuseMap, or is the colour without it
	float  Shininess;
	float  Opacity; // 1 unless the material is blended (MaterialInstance::BuildInstanceData)
};

// materialA, materialB: GraphicsLayouts::InstanceMaterialLayout's registers, as an instance stream or a constant
// buffer holds them
InstanceMaterial UnpackInstanceMaterial(float4 materialA, float4 materialB)
{
	InstanceMaterial material;
	material.Tint = materialA.xyz;
	material.Shininess = materialA.w;
	material.Opacity = materialB.x;
	return material;
}

// Shared VS output / PS input for the forward pass. All vectors are world space.
struct VertexToPixel
{
	float4 Position  : SV_POSITION;
	float3 Normal    : NORMAL;
	float2 UV        : UV;
	float3 Tangent   : TANGENT;
	float3 WorldPos  : POSITION;
	float3 CameraPos : POSITION1;
	// The draw's InstanceMaterial
	nointerpolation float3 Tint      : TINT;
	nointerpolation float  Shininess : SHININESS;
	nointerpolation float  Opacity   : OPACITY;
};

// What a depth-only pass reads of a mesh vertex (Vertex_MeshDepthOnly): where it is, and its UV for cutouts' masks. The
// same buffer as VertexInput's; the input layout names only these two (MeshInstanceFormats::Depth).
struct DepthVertexInput
{
	float3 Position : POSITION;
	float2 UV       : UV;
};

// Vertex_MeshDepthOnly's output, which Pixel_DepthOnlyCutout reads
struct DepthVertexToPixel
{
	float4 Position : SV_POSITION;
	float2 UV       : UV;
};

// Vertex_MeshID's output, which Pixel_PickID and Pixel_OutlineMask read: DepthVertexToPixel's, and the ID the stage
// gave the draw (low half, high half)
struct IDVertexToPixel
{
	float4 Position          : SV_POSITION;
	float2 UV                : UV;
	nointerpolation uint2 ID : DRAWID;
};

#endif // FORWARD_TYPES_HLSLI
