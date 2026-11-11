#ifndef GTAO_HLSLI
#define GTAO_HLSLI

// Ground Truth Ambient Occlusion, ported from Intel's XeGTAO (MIT licence, github.com/GameTechDev/XeGTAO), after
// Jimenez et al., "Practical Real-Time Strategies for Accurate Indirect Occlusion" (2016). Shared by the three
// Compute_GTAO* passes of AmbientOcclusionStage.
//
// Differences from XeGTAO:
// - Every texture is read with Load at a pixel clamped to the viewport, not sampled by UV. A view can be the top-left
//   part of a bigger target (RenderView::ViewportSize), and pixels mean the same in both.
// - Normals are always made from depth, in place, and there are no bent normals.
// - The noise doesn't change over frames: there's no TAA to average it, so the denoise passes do instead.
//
// View space here is XeGTAO's, built from depth alone: x right, y up, z the distance in front of the camera.

// Mirrors AmbientOcclusionStage::ParamsLayout
cbuffer GTAOParams : register(b0)
{
	float2 ViewportSize;
	float2 ViewportPixelSize;           // 1 / ViewportSize
	float2 DepthUnpackConsts;           // Device depth to distance: x / (y - depth)
	float2 CameraTanHalfFOV;
	float2 NDCToViewMul;
	float2 NDCToViewAdd;
	float2 NDCToViewMul_x_PixelSize;
	float  EffectRadius;                // Metres
	float  EffectFalloffRange;          // The outer fraction of the radius that fades out
	float  RadiusMultiplier;
	float  FinalValuePower;
	float  DenoiseBlurBeta;             // The centre's weight in the denoise
	float  SampleDistributionPower;     // > 1 puts more samples near the centre
	float  ThinOccluderCompensation;
	float  DepthMIPSamplingOffset;
	float  SliceCount;
	float  StepsPerSlice;
	float  FinalApply;                  // 1 on the last denoise pass, which writes the finished value
	// Three scalars rather than a float3: GLSL can't place a vector at this offset, and the layout is the same
	float  _Pad0;
	float  _Pad1;
	float  _Pad2;
};

#define GTAO_PI                 3.1415926535897932384626433832795
#define GTAO_PI_HALF            1.5707963267948966192313216916398
// The depth MIP chain: mip 0 plus 4 smaller ones
#define GTAO_DEPTH_MIP_LEVELS   5
// The raw term can go a little over 1 before it's averaged, so it's stored divided by this to fit UNORM
#define GTAO_OCCLUSION_TERM_SCALE 1.5

// Loads with the viewport's clamp-to-edge, at a mip of a texture whose mip 0 is the viewport's size or bigger
int2 ClampToViewport(int2 pixel, uint mip)
{
	const int2 lastPixel = max(int2(ViewportSize) - 1, 0) >> mip;
	return clamp(pixel, int2(0, 0), lastPixel);
}

// Device depth (0 at the near plane, 1 at the far plane) to the distance in front of the camera
float ScreenSpaceToViewSpaceDepth(float screenDepth)
{
	return DepthUnpackConsts.x / (DepthUnpackConsts.y - screenDepth);
}

float ClampDepth(float depth)
{
	return clamp(depth, 0.0f, 3.402823466e+38f);
}

// screenPos is 0 to 1 across the viewport, y down
float3 ComputeViewspacePosition(float2 screenPos, float viewspaceDepth)
{
	float3 position;
	position.xy = (NDCToViewMul * screenPos + NDCToViewAdd) * viewspaceDepth;
	position.z = viewspaceDepth;
	return position;
}

// Input -1 to 1, output 0 to pi (Lagarde's fit)
float FastACos(float inX)
{
	const float x = abs(inX);
	float result = -0.156583f * x + GTAO_PI_HALF;
	result *= sqrt(1.0f - x);
	return inX >= 0.0f ? result : GTAO_PI - result;
}

// Weighted towards the nearest of four depths, so a mip doesn't average a near edge into the background behind it
float DepthMIPFilter(float depth0, float depth1, float depth2, float depth3)
{
	const float maxDepth = max(max(depth0, depth1), max(depth2, depth3));

	const float depthRangeScaleFactor = 0.75f; // XeGTAO's, found empirically
	const float effectRadius = depthRangeScaleFactor * EffectRadius * RadiusMultiplier;
	const float falloffRange = EffectFalloffRange * effectRadius;
	const float falloffFrom = effectRadius * (1.0f - EffectFalloffRange);
	const float falloffMul = -1.0f / falloffRange;
	const float falloffAdd = falloffFrom / falloffRange + 1.0f;

	const float weight0 = saturate((maxDepth - depth0) * falloffMul + falloffAdd);
	const float weight1 = saturate((maxDepth - depth1) * falloffMul + falloffAdd);
	const float weight2 = saturate((maxDepth - depth2) * falloffMul + falloffAdd);
	const float weight3 = saturate((maxDepth - depth3) * falloffMul + falloffAdd);

	const float weightSum = weight0 + weight1 + weight2 + weight3;
	return (weight0 * depth0 + weight1 * depth1 + weight2 * depth2 + weight3 * depth3) / weightSum;
}

// How much each neighbour (left, right, top, bottom) is on the same surface: 1 is, 0 is across a depth edge.
// Slope-aware, so a steep but continuous surface isn't an edge.
float4 CalculateEdges(float centerZ, float leftZ, float rightZ, float topZ, float bottomZ)
{
	float4 edgesLRTB = float4(leftZ, rightZ, topZ, bottomZ) - centerZ;

	const float slopeLR = (edgesLRTB.y - edgesLRTB.x) * 0.5f;
	const float slopeTB = (edgesLRTB.w - edgesLRTB.z) * 0.5f;
	const float4 edgesLRTBSlopeAdjusted = edgesLRTB + float4(slopeLR, -slopeLR, slopeTB, -slopeTB);
	edgesLRTB = min(abs(edgesLRTB), abs(edgesLRTBSlopeAdjusted));
	return saturate(1.25f - edgesLRTB / (centerZ * 0.011f));
}

// Four 2-bit edges in one UNORM8
float PackEdges(float4 edgesLRTB)
{
	edgesLRTB = round(saturate(edgesLRTB) * 2.9f);
	return dot(edgesLRTB, float4(64.0f / 255.0f, 16.0f / 255.0f, 4.0f / 255.0f, 1.0f / 255.0f));
}

float4 UnpackEdges(float packedValue)
{
	const uint packed = (uint)(packedValue * 255.5f);
	float4 edgesLRTB;
	edgesLRTB.x = float((packed >> 6) & 0x03) / 3.0f;
	edgesLRTB.y = float((packed >> 4) & 0x03) / 3.0f;
	edgesLRTB.z = float((packed >> 2) & 0x03) / 3.0f;
	edgesLRTB.w = float((packed >> 0) & 0x03) / 3.0f;
	return saturate(edgesLRTB);
}

// The view-space normal from the four neighbours' positions, leaving out those across an edge
float3 CalculateNormal(float4 edgesLRTB, float3 pixCenterPos, float3 pixLPos, float3 pixRPos, float3 pixTPos, float3 pixBPos)
{
	const float4 acceptedNormals = saturate(float4(edgesLRTB.x * edgesLRTB.z, edgesLRTB.z * edgesLRTB.y, edgesLRTB.y * edgesLRTB.w, edgesLRTB.w * edgesLRTB.x) + 0.01f);

	pixLPos = normalize(pixLPos - pixCenterPos);
	pixRPos = normalize(pixRPos - pixCenterPos);
	pixTPos = normalize(pixTPos - pixCenterPos);
	pixBPos = normalize(pixBPos - pixCenterPos);

	const float3 pixelNormal = acceptedNormals.x * cross(pixLPos, pixTPos)
		+ acceptedNormals.y * cross(pixTPos, pixRPos)
		+ acceptedNormals.z * cross(pixRPos, pixBPos)
		+ acceptedNormals.w * cross(pixBPos, pixLPos);
	return normalize(pixelNormal);
}

// A pixel's place along a Hilbert curve through a 64x64 tile, so neighbouring pixels get well-spread noise
uint HilbertIndex(uint posX, uint posY)
{
	const uint width = 64;
	uint index = 0;
	for (uint curLevel = width / 2; curLevel > 0; curLevel /= 2)
	{
		const uint regionX = (posX & curLevel) > 0 ? 1 : 0;
		const uint regionY = (posY & curLevel) > 0 ? 1 : 0;
		index += curLevel * curLevel * ((3 * regionX) ^ regionY);
		if (regionY == 0)
		{
			if (regionX == 1)
			{
				posX = (width - 1) - posX;
				posY = (width - 1) - posY;
			}

			const uint temp = posX;
			posX = posY;
			posY = temp;
		}
	}
	return index;
}

// Two values 0 to 1 per pixel: the R2 sequence along the Hilbert curve. Fixed over time (no TAA).
float2 SpatialNoise(uint2 pixCoord)
{
	const uint index = HilbertIndex(pixCoord.x % 64, pixCoord.y % 64);
	return frac(0.5f + index * float2(0.75487766624669276005f, 0.5698402909980532659114f));
}

#endif // GTAO_HLSLI
