// GTAO pass 1 (AmbientOcclusionStage): the scene's device depth to view-space distance, and 4 smaller mips of it.
// Each thread takes a 2x2 block, so a group of 8x8 covers 16x16 pixels and writes mips 0 to 4 of them.
#include "Common/GTAO.hlsli"

Texture2D<float> SourceDepth : register(t0); // The view's depth (DepthPrepassStage)

RWTexture2D<float> OutDepth0 : register(u0);
RWTexture2D<float> OutDepth1 : register(u1);
RWTexture2D<float> OutDepth2 : register(u2);
RWTexture2D<float> OutDepth3 : register(u3);
RWTexture2D<float> OutDepth4 : register(u4);

groupshared float g_scratchDepths[8][8];

float LoadViewspaceDepth(int2 pixel)
{
	return ClampDepth(ScreenSpaceToViewSpaceDepth(SourceDepth.Load(int3(ClampToViewport(pixel, 0), 0))));
}

[numthreads(8, 8, 1)]
void CSMain(uint2 dispatchThreadID : SV_DispatchThreadID, uint2 groupThreadID : SV_GroupThreadID)
{
	// Mip 0
	const uint2 baseCoord = dispatchThreadID;
	const int2 pixCoord = int2(baseCoord * 2);
	const float depth0 = LoadViewspaceDepth(pixCoord + int2(0, 0));
	const float depth1 = LoadViewspaceDepth(pixCoord + int2(1, 0));
	const float depth2 = LoadViewspaceDepth(pixCoord + int2(0, 1));
	const float depth3 = LoadViewspaceDepth(pixCoord + int2(1, 1));
	OutDepth0[pixCoord + int2(0, 0)] = depth0;
	OutDepth0[pixCoord + int2(1, 0)] = depth1;
	OutDepth0[pixCoord + int2(0, 1)] = depth2;
	OutDepth0[pixCoord + int2(1, 1)] = depth3;

	// Mip 1
	const float dm1 = DepthMIPFilter(depth0, depth1, depth2, depth3);
	OutDepth1[baseCoord] = dm1;
	g_scratchDepths[groupThreadID.x][groupThreadID.y] = dm1;

	GroupMemoryBarrierWithGroupSync();

	// Mip 2
	[branch]
	if (all((groupThreadID.xy % 2) == 0))
	{
		const float inTL = g_scratchDepths[groupThreadID.x + 0][groupThreadID.y + 0];
		const float inTR = g_scratchDepths[groupThreadID.x + 1][groupThreadID.y + 0];
		const float inBL = g_scratchDepths[groupThreadID.x + 0][groupThreadID.y + 1];
		const float inBR = g_scratchDepths[groupThreadID.x + 1][groupThreadID.y + 1];

		const float dm2 = DepthMIPFilter(inTL, inTR, inBL, inBR);
		OutDepth2[baseCoord / 2] = dm2;
		g_scratchDepths[groupThreadID.x][groupThreadID.y] = dm2;
	}

	GroupMemoryBarrierWithGroupSync();

	// Mip 3
	[branch]
	if (all((groupThreadID.xy % 4) == 0))
	{
		const float inTL = g_scratchDepths[groupThreadID.x + 0][groupThreadID.y + 0];
		const float inTR = g_scratchDepths[groupThreadID.x + 2][groupThreadID.y + 0];
		const float inBL = g_scratchDepths[groupThreadID.x + 0][groupThreadID.y + 2];
		const float inBR = g_scratchDepths[groupThreadID.x + 2][groupThreadID.y + 2];

		const float dm3 = DepthMIPFilter(inTL, inTR, inBL, inBR);
		OutDepth3[baseCoord / 4] = dm3;
		g_scratchDepths[groupThreadID.x][groupThreadID.y] = dm3;
	}

	GroupMemoryBarrierWithGroupSync();

	// Mip 4
	[branch]
	if (all((groupThreadID.xy % 8) == 0))
	{
		const float inTL = g_scratchDepths[groupThreadID.x + 0][groupThreadID.y + 0];
		const float inTR = g_scratchDepths[groupThreadID.x + 4][groupThreadID.y + 0];
		const float inBL = g_scratchDepths[groupThreadID.x + 0][groupThreadID.y + 4];
		const float inBR = g_scratchDepths[groupThreadID.x + 4][groupThreadID.y + 4];

		const float dm4 = DepthMIPFilter(inTL, inTR, inBL, inBR);
		OutDepth4[baseCoord / 8] = dm4;
	}
}
