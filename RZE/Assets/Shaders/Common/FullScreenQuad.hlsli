#ifndef FULLSCREENQUAD_HLSLI
#define FULLSCREENQUAD_HLSLI

// Output of Vertex_RenderTargetQuad.hlsl, which every full-screen pass (DrawFullScreenPass) draws with. UV runs 0 to 1
// across the viewport; Position is the pixel's centre in the target, which the viewport is the top-left part of.
struct PS_IN
{
	float4 Position : SV_POSITION;
	float2 UV       : UV;
};

#endif // FULLSCREENQUAD_HLSLI
