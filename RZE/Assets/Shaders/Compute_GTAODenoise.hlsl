// GTAO pass 3 (AmbientOcclusionStage): an edge-aware 3x3 blur of the visibility, run once or twice. Neighbours across a
// depth edge (from pass 2) don't blend in. The last pass (FinalApply) writes the finished visibility, 0 to 1.
#include "Common/GTAO.hlsli"

Texture2D<float> SourceAOTerm : register(t0); // Visibility / GTAO_OCCLUSION_TERM_SCALE
Texture2D<float> SourceEdges : register(t1);  // PackEdges

RWTexture2D<unorm float> OutAOTerm : register(u0);

float LoadAO(int2 pixel)
{
	return SourceAOTerm.Load(int3(ClampToViewport(pixel, 0), 0));
}

float4 LoadEdges(int2 pixel)
{
	return UnpackEdges(SourceEdges.Load(int3(ClampToViewport(pixel, 0), 0)));
}

void AddSample(float ssaoValue, float edgeValue, inout float sum, inout float sumWeight)
{
	const float weight = edgeValue;
	sum += weight * ssaoValue;
	sumWeight += weight;
}

[numthreads(8, 8, 1)]
void CSMain(uint2 pixCoord : SV_DispatchThreadID)
{
	if (any(pixCoord >= uint2(ViewportSize)))
	{
		return;
	}

	const bool finalApply = FinalApply > 0.5f;
	const float blurAmount = finalApply ? DenoiseBlurBeta : DenoiseBlurBeta / 5.0f;
	const float diagWeight = 0.85f * 0.5f;

	const int2 pixel = int2(pixCoord);
	float4 edgesC_LRTB = LoadEdges(pixel);
	const float4 edgesL_LRTB = LoadEdges(pixel + int2(-1, 0));
	const float4 edgesR_LRTB = LoadEdges(pixel + int2(1, 0));
	const float4 edgesT_LRTB = LoadEdges(pixel + int2(0, -1));
	const float4 edgesB_LRTB = LoadEdges(pixel + int2(0, 1));

	// Edge detection isn't symmetrical: a neighbour can see an edge this pixel doesn't. Either one seeing it counts.
	edgesC_LRTB *= float4(edgesL_LRTB.y, edgesR_LRTB.x, edgesT_LRTB.w, edgesB_LRTB.z);

	// With three or four edges, a little leaks in from the neighbours anyway, against aliasing
	const float leakThreshold = 2.5f;
	const float leakStrength = 0.5f;
	const float edginess = (saturate(4.0f - leakThreshold - dot(edgesC_LRTB, 1.0f.xxxx)) / (4.0f - leakThreshold)) * leakStrength;
	edgesC_LRTB = saturate(edgesC_LRTB + edginess);

	const float weightTL = diagWeight * (edgesC_LRTB.x * edgesL_LRTB.z + edgesC_LRTB.z * edgesT_LRTB.x);
	const float weightTR = diagWeight * (edgesC_LRTB.z * edgesT_LRTB.y + edgesC_LRTB.y * edgesR_LRTB.z);
	const float weightBL = diagWeight * (edgesC_LRTB.w * edgesB_LRTB.x + edgesC_LRTB.x * edgesL_LRTB.w);
	const float weightBR = diagWeight * (edgesC_LRTB.y * edgesR_LRTB.w + edgesC_LRTB.w * edgesB_LRTB.y);

	float sumWeight = blurAmount;
	float sum = LoadAO(pixel) * sumWeight;

	AddSample(LoadAO(pixel + int2(-1, 0)), edgesC_LRTB.x, sum, sumWeight);
	AddSample(LoadAO(pixel + int2(1, 0)), edgesC_LRTB.y, sum, sumWeight);
	AddSample(LoadAO(pixel + int2(0, -1)), edgesC_LRTB.z, sum, sumWeight);
	AddSample(LoadAO(pixel + int2(0, 1)), edgesC_LRTB.w, sum, sumWeight);

	AddSample(LoadAO(pixel + int2(-1, -1)), weightTL, sum, sumWeight);
	AddSample(LoadAO(pixel + int2(1, -1)), weightTR, sum, sumWeight);
	AddSample(LoadAO(pixel + int2(-1, 1)), weightBL, sum, sumWeight);
	AddSample(LoadAO(pixel + int2(1, 1)), weightBR, sum, sumWeight);

	float aoTerm = sum / sumWeight;
	if (finalApply)
	{
		aoTerm *= GTAO_OCCLUSION_TERM_SCALE;
	}

	OutAOTerm[pixCoord] = saturate(aoTerm);
}
