// Probe capture (ProbeCapture): draws a mesh into each probe face of the batch that sees it, an instance a face. Each face
// is a 90-degree view from its probe, squeezed into its square of the target, with clip distances at the square's
// edges so a triangle crossing them stops there rather than drawing onto the next face.
#include "Common/ProbeCaptureTypes.hlsli"
#include "Common/VertexResources.hlsli"

// Mirrors ProbeCapture::k_maxBatchFaces
#define MAX_CAPTURE_FACES 384

// Mirrors ProbeCapture::FaceLayout: one face of a probe in the batch
struct CaptureFace
{
	float4x4 ViewProjection;
	// Where the face is in the target, in its clip space: x and y scale, then x and y offset
	float4 Placement;
	// xyz: the probe's
	float4 ProbePosition;
};

// Mirrors ProbeCapture::FaceTableLayout. After VertexResources.hlsli's b0 and b1.
cbuffer CaptureFaces : register(b2)
{
	CaptureFace Faces[MAX_CAPTURE_FACES];
};

CaptureVertexToPixel VSMain(CaptureVertexInput input)
{
	const CaptureFace face = Faces[input.Face];

	CaptureVertexToPixel output;
	const float4 worldPos = mul(World, float4(input.Position, 1.0f));
	const float4 faceClip = mul(face.ViewProjection, worldPos);

	// Inside the face's own view, -w to w across and up
	output.FaceEdges = float4(faceClip.w + faceClip.x, faceClip.w - faceClip.x, faceClip.w + faceClip.y, faceClip.w - faceClip.y);
	output.Position = float4(faceClip.xy * face.Placement.xy + face.Placement.zw * faceClip.w, faceClip.zw);

	// Normals transform by the inverse-transpose so non-uniform scale doesn't skew them
	output.Normal = normalize(mul(input.Normal, (float3x3)InvWorld));
	output.UV = input.UV;
	output.WorldPos = worldPos.xyz;
	output.ProbePosition = face.ProbePosition.xyz;
	return output;
}
