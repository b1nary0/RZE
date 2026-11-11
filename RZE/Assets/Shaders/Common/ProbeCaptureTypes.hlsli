#ifndef PROBE_CAPTURE_TYPES_HLSLI
#define PROBE_CAPTURE_TYPES_HLSLI

// What a batch of probe captures (ProbeCapture) passes from Vertex_ProbeCapture.hlsl to Pixel_ProbeCapture.hlsl: every
// face of every probe in the batch, side by side in one target, each draw instanced once per face that sees it.

// The forward vertex layout (ForwardTypes.hlsli's VertexInput), and which face each instance is drawn into from a
// second, per-instance stream
struct CaptureVertexInput
{
	float3 Position : POSITION;
	float3 Normal   : NORMAL;
	float2 UV       : UV;
	float3 Tangent  : TANGENT;
	uint   Face     : FACE;
};

struct CaptureVertexToPixel
{
	float4 Position : SV_POSITION;
	// Positive inside the face's square, so nothing spills onto the faces beside it
	float4 FaceEdges : SV_ClipDistance0;
	float3 Normal   : NORMAL;
	float2 UV       : UV;
	float3 WorldPos : POSITION;
	nointerpolation float3 ProbePosition : POSITION1;
};

#endif // PROBE_CAPTURE_TYPES_HLSLI
