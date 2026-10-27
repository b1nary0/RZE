// GTAO pass 2 (AmbientOcclusionStage): for each pixel, a few slices through the hemisphere around the view direction.
// Each slice searches the depth both ways for the highest horizon and integrates the visible arc, cosine-weighted,
// against the normal (lines from Algorithm 1 of the paper are noted). Writes the raw visibility, and the depth edges for
// the denoise.
#include "Common/GTAO.hlsli"

Texture2D<float> ViewspaceDepth : register(t0); // From pass 1, with GTAO_DEPTH_MIP_LEVELS mips

RWTexture2D<unorm float> OutWorkingAOTerm : register(u0); // Visibility / GTAO_OCCLUSION_TERM_SCALE
RWTexture2D<unorm float> OutWorkingEdges : register(u1);  // PackEdges

float LoadDepth(int2 pixel, uint mip)
{
	return ViewspaceDepth.Load(int3(ClampToViewport(pixel, mip), mip));
}

// What XeGTAO's point-sampled SampleLevel reads: the texel under screenPos at the nearest mip
float SampleDepth(float2 screenPos, float mipLevel)
{
	const uint mip = (uint)round(mipLevel);
	const int2 pixel = int2(floor(screenPos * ViewportSize)) >> mip;
	return LoadDepth(pixel, mip);
}

[numthreads(8, 8, 1)]
void CSMain(uint2 pixCoord : SV_DispatchThreadID)
{
	if (any(pixCoord >= uint2(ViewportSize)))
	{
		return;
	}

	const int2 pixel = int2(pixCoord);
	const float2 normalizedScreenPos = (float2(pixCoord) + 0.5f) * ViewportPixelSize;

	float viewspaceZ = LoadDepth(pixel, 0);
	const float pixLZ = LoadDepth(pixel + int2(-1, 0), 0);
	const float pixTZ = LoadDepth(pixel + int2(0, -1), 0);
	const float pixRZ = LoadDepth(pixel + int2(1, 0), 0);
	const float pixBZ = LoadDepth(pixel + int2(0, 1), 0);

	const float4 edgesLRTB = CalculateEdges(viewspaceZ, pixLZ, pixRZ, pixTZ, pixBZ);
	OutWorkingEdges[pixCoord] = PackEdges(edgesLRTB);

	const float3 CENTER = ComputeViewspacePosition(normalizedScreenPos, viewspaceZ);
	const float3 LEFT = ComputeViewspacePosition(normalizedScreenPos + float2(-1, 0) * ViewportPixelSize, pixLZ);
	const float3 RIGHT = ComputeViewspacePosition(normalizedScreenPos + float2(1, 0) * ViewportPixelSize, pixRZ);
	const float3 TOP = ComputeViewspacePosition(normalizedScreenPos + float2(0, -1) * ViewportPixelSize, pixTZ);
	const float3 BOTTOM = ComputeViewspacePosition(normalizedScreenPos + float2(0, 1) * ViewportPixelSize, pixBZ);
	const float3 viewspaceNormal = CalculateNormal(edgesLRTB, CENTER, LEFT, RIGHT, TOP, BOTTOM);

	// Moved slightly towards the camera, so depth imprecision doesn't occlude the surface with itself
	viewspaceZ *= 0.99999f;

	const float3 pixCenterPos = ComputeViewspacePosition(normalizedScreenPos, viewspaceZ);
	const float3 viewVec = normalize(-pixCenterPos);

	const float effectRadius = EffectRadius * RadiusMultiplier;
	const float falloffRange = EffectFalloffRange * effectRadius;
	const float falloffFrom = effectRadius * (1.0f - EffectFalloffRange);
	const float falloffMul = -1.0f / falloffRange;
	const float falloffAdd = falloffFrom / falloffRange + 1.0f;

	const float2 localNoise = SpatialNoise(pixCoord);
	const float noiseSlice = localNoise.x;
	const float noiseSample = localNoise.y;

	// A sample closer than about a pixel is pushed out to that distance: the centre pixel itself tells nothing
	const float pixelTooCloseThreshold = 1.3f;

	// The view-space size of a pixel at the centre's depth
	const float2 pixelDirRBViewspaceSizeAtCenterZ = viewspaceZ.xx * NDCToViewMul_x_PixelSize;
	const float screenspaceRadius = effectRadius / pixelDirRBViewspaceSizeAtCenterZ.x;

	// Fades out where the radius is only a few pixels across
	float visibility = saturate((10.0f - screenspaceRadius) / 100.0f) * 0.5f;

	const float minS = pixelTooCloseThreshold / screenspaceRadius;

	for (float slice = 0.0f; slice < SliceCount; slice++)
	{
		const float sliceK = (slice + noiseSlice) / SliceCount;
		// Lines 5, 6
		const float phi = sliceK * GTAO_PI;
		const float cosPhi = cos(phi);
		const float sinPhi = sin(phi);
		// In pixels
		const float2 omega = float2(cosPhi, -sinPhi) * screenspaceRadius;

		// Line 8
		const float3 directionVec = float3(cosPhi, sinPhi, 0.0f);
		// Line 9
		const float3 orthoDirectionVec = directionVec - (dot(directionVec, viewVec) * viewVec);
		// Line 10: orthogonal to the direction and the view, to project the normal onto the slice
		const float3 axisVec = normalize(cross(orthoDirectionVec, viewVec));
		// Line 11
		const float3 projectedNormalVec = viewspaceNormal - axisVec * dot(viewspaceNormal, axisVec);
		// Line 13
		const float signNorm = sign(dot(orthoDirectionVec, projectedNormalVec));
		// Line 14
		float projectedNormalVecLength = length(projectedNormalVec);
		const float cosNorm = saturate(dot(projectedNormalVec, viewVec) / projectedNormalVecLength);
		// Line 15
		const float n = signNorm * FastACos(cosNorm);

		// The lowest horizons: the normal's tangent plane on each side
		const float lowHorizonCos0 = cos(n + GTAO_PI_HALF);
		const float lowHorizonCos1 = cos(n - GTAO_PI_HALF);

		// Lines 17, 18, both sides at once
		float horizonCos0 = lowHorizonCos0;
		float horizonCos1 = lowHorizonCos1;

		for (float step = 0.0f; step < StepsPerSlice; step++)
		{
			// R1 sequence along the steps
			const float stepBaseNoise = float(slice + step * StepsPerSlice) * 0.6180339887498948482f;
			const float stepNoise = frac(noiseSample + stepBaseNoise);

			// Line 20, with noise
			float s = (step + stepNoise) / StepsPerSlice;
			// s is never negative; the max tells the compiler so
			s = pow(max(s, 0.0f), SampleDistributionPower);
			s += minS;

			// Lines 21, 22
			float2 sampleOffset = s * omega;
			const float sampleOffsetLength = length(sampleOffset);

			// Further samples read coarser mips
			const float mipLevel = clamp(log2(sampleOffsetLength) - DepthMIPSamplingOffset, 0.0f, GTAO_DEPTH_MIP_LEVELS - 1);

			// Snapped to a pixel's centre, so the direction matches the depth that's read
			sampleOffset = round(sampleOffset) * ViewportPixelSize;

			const float2 sampleScreenPos0 = normalizedScreenPos + sampleOffset;
			const float SZ0 = SampleDepth(sampleScreenPos0, mipLevel);
			const float3 samplePos0 = ComputeViewspacePosition(sampleScreenPos0, SZ0);

			const float2 sampleScreenPos1 = normalizedScreenPos - sampleOffset;
			const float SZ1 = SampleDepth(sampleScreenPos1, mipLevel);
			const float3 samplePos1 = ComputeViewspacePosition(sampleScreenPos1, SZ1);

			const float3 sampleDelta0 = samplePos0 - pixCenterPos;
			const float3 sampleDelta1 = samplePos1 - pixCenterPos;
			const float sampleDist0 = length(sampleDelta0);
			const float sampleDist1 = length(sampleDelta1);

			// Lines 23, 24
			const float3 sampleHorizonVec0 = sampleDelta0 / sampleDist0;
			const float3 sampleHorizonVec1 = sampleDelta1 / sampleDist1;

			// Samples beyond the radius don't count, fading out over the falloff range; samples behind the centre fade
			// sooner with thin-occluder compensation
			const float falloffBase0 = length(float3(sampleDelta0.x, sampleDelta0.y, sampleDelta0.z * (1.0f + ThinOccluderCompensation)));
			const float falloffBase1 = length(float3(sampleDelta1.x, sampleDelta1.y, sampleDelta1.z * (1.0f + ThinOccluderCompensation)));
			const float weight0 = saturate(falloffBase0 * falloffMul + falloffAdd);
			const float weight1 = saturate(falloffBase1 * falloffMul + falloffAdd);

			float shc0 = dot(sampleHorizonVec0, viewVec);
			float shc1 = dot(sampleHorizonVec1, viewVec);
			shc0 = lerp(lowHorizonCos0, shc0, weight0);
			shc1 = lerp(lowHorizonCos1, shc1, weight1);

			horizonCos0 = max(horizonCos0, shc0);
			horizonCos1 = max(horizonCos1, shc1);
		}

		// XeGTAO's fudge for slight over-darkening on steep slopes
		projectedNormalVecLength = lerp(projectedNormalVecLength, 1.0f, 0.05f);

		// Line 27
		const float h0 = -FastACos(horizonCos1);
		const float h1 = FastACos(horizonCos0);
		const float iarc0 = (cosNorm + 2.0f * h0 * sin(n) - cos(2.0f * h0 - n)) / 4.0f;
		const float iarc1 = (cosNorm + 2.0f * h1 * sin(n) - cos(2.0f * h1 - n)) / 4.0f;
		visibility += projectedNormalVecLength * (iarc0 + iarc1);
	}

	visibility /= SliceCount;
	visibility = pow(max(visibility, 0.0f), FinalValuePower);
	// Never fully occluded: the pixel is visible, after all
	visibility = max(0.03f, visibility);

	OutWorkingAOTerm[pixCoord] = saturate(visibility / GTAO_OCCLUSION_TERM_SCALE);
}
