#ifndef SMAA_HLSLI
#define SMAA_HLSLI

// SMAA 1x (Jimenez et al., SMAA.hlsl beside this file) set up for the engine: its texture and sampler syntax, its
// registers, and the view's preset values. Drawn by AntiAliasingStage, three passes: Pixel_SMAAEdgeDetection,
// Pixel_SMAABlendingWeights(NoDiagonals) and Pixel_SMAANeighborhoodBlending.
//
// SMAA.hlsl is upstream iryoku/smaa at 71c806a8 (2013-11-06), byte for byte: CRLF line endings and one Latin-1 byte
// (0xB7, in a comment diagram), unlike the other shaders. Don't edit or normalise it, so it stays diffable with
// upstream; set things up here instead.
//
// SMAA_HLSL_4's own setup declares its samplers in Effects syntax without registers, so the porting macros are ours
// (SMAA_CUSTOM_SL).
#include "../FullScreenQuad.hlsli"

// Must match AntiAliasingStage::SMAAParamsLayout
cbuffer SMAAParams : register(b0)
{
	float4 RTMetrics;          // 1 / width, 1 / height, width, height of the target (not the viewport)
	float  Threshold;          // Less luma contrast than this isn't an edge
	float  MaxSearchSteps;     // How far the search for an edge's ends goes, in steps of 2 pixels
	float  MaxSearchStepsDiag; // The same along diagonal edges; unused without diagonal detection
	float  CornerRounding;     // How much sharp corners are kept, 0 to 100; unused without corner detection
};

// Linear and point, both clamped (Renderer.h's sampler registers)
SamplerState LinearSampler : register(s2);
SamplerState PointSampler : register(s3);

#define SMAA_CUSTOM_SL 1
#define SMAATexture2D(tex) Texture2D tex
#define SMAATexturePass2D(tex) tex
#define SMAASampleLevelZero(tex, coord) tex.SampleLevel(LinearSampler, coord, 0)
#define SMAASampleLevelZeroPoint(tex, coord) tex.SampleLevel(PointSampler, coord, 0)
#define SMAASampleLevelZeroOffset(tex, coord, offset) tex.SampleLevel(LinearSampler, coord, 0, offset)
#define SMAASample(tex, coord) tex.Sample(LinearSampler, coord)
#define SMAASamplePoint(tex, coord) tex.Sample(PointSampler, coord)
#define SMAASampleOffset(tex, coord, offset) tex.Sample(LinearSampler, coord, offset)
#define SMAA_FLATTEN [flatten]
#define SMAA_BRANCH [branch]

// The preset's values come from the view each frame (GetSMAASettings), so one shader serves every quality. Only
// diagonal and corner detection are compile-time: Pixel_SMAABlendingWeightsNoDiagonals turns them off.
#define SMAA_RT_METRICS RTMetrics
#define SMAA_THRESHOLD Threshold
#define SMAA_MAX_SEARCH_STEPS MaxSearchSteps
#define SMAA_MAX_SEARCH_STEPS_DIAG MaxSearchStepsDiag
#define SMAA_CORNER_ROUNDING CornerRounding

#include "SMAA.hlsl"

// The pixel's centre in the target's texture coordinates. Not the quad's UV, which spans the viewport, the target's
// top-left part.
float2 SMAATexcoord(PS_IN input)
{
	return input.Position.xy * SMAA_RT_METRICS.xy;
}

#endif // SMAA_HLSLI
