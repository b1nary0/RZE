#ifndef LIGHTING_HLSLI
#define LIGHTING_HLSLI

// Include paths are relative to Assets/Shaders/: the engine's D3D_COMPILE_STANDARD_FILE_INCLUDE
// resolves nested includes from the top-level shader's directory, not the including file's.
#include "Common/PixelResources.hlsli"

//
// Tunables
//
static const float  AlphaCutoff  = 0.5f;
static const float  Exposure     = 1.0f;
// Height-map bump: normal tilt per unit of height change per texel
static const float  BumpStrength = 10.0f;
// Linear-space hemisphere ambient: sky colour for up-facing normals, ground colour for down-facing.
// Kept well below the light strength (~2) so lit and unlit sides read clearly.
static const float3 AmbientSky    = float3(0.14f, 0.16f, 0.20f);
static const float3 AmbientGround = float3(0.05f, 0.045f, 0.04f);
// Specular reflectance at normal incidence = SpecularIntensity * this; 0.5 intensity gives the typical dielectric 0.04
static const float  SpecularF0Scale = 0.08f;

//
// Colour space
//
// Textures and render targets are UNORM holding sRGB-encoded data, so convert manually.
float3 SRGBToLinear(float3 colour)
{
	return pow(max(colour, 0.0f), 2.2f);
}

float3 LinearToSRGB(float3 colour)
{
	return pow(max(colour, 0.0f), 1.0f / 2.2f);
}

// ACES filmic curve fit (Krzysztof Narkowicz). Keeps more contrast and saturation than Reinhard.
float3 TonemapACES(float3 colour)
{
	const float a = 2.51f;
	const float b = 0.03f;
	const float c = 2.43f;
	const float d = 0.59f;
	const float e = 0.14f;
	return saturate((colour * (a * colour + b)) / (colour * (c * colour + d) + e));
}

// Linear HDR lighting result -> display-ready colour for the UNORM render target
float3 FinalizeColour(float3 linearColour)
{
	return LinearToSRGB(TonemapACES(linearColour * Exposure));
}

//
// Surface
//
struct SurfaceData
{
	float3 Albedo;            // Linear
	float3 Normal;            // World space, normalized; includes normal/height mapping
	float3 VertexNormal;      // World space, normalized; geometric normal, used for shadow lookup offset
	float  SpecularIntensity; // 0..1 specular mask; 0.5 is a typical non-metal
	float  Shininess;
	float3 WorldPos;
};

// Discards the pixel if the material's cutout mask says so. Diffuse alpha isn't used for this
// because some assets (e.g. Nanosuit) store unrelated data there.
void ApplyOpacityMask(float2 uv)
{
	if (HasOpacityMask > 0.5f)
	{
		float4 mask = OpacityMap.Sample(LinearSampler, uv);
		clip((OpacityMaskInAlpha > 0.5f ? mask.a : mask.r) - AlphaCutoff);
	}
}

// Applies a tangent-space normal map sample to the world-space vertex normal
float3 PerturbNormal(float3 normal, float3 tangent, float3 normalMapSample)
{
	// Gram-Schmidt: re-orthogonalize the interpolated tangent against the normal
	tangent = normalize(tangent - dot(tangent, normal) * normal);
	float3 bitangent = cross(tangent, normal);

	float3 tangentSpaceNormal = normalMapSample * 2.0f - 1.0f;
	float3x3 TBN = float3x3(tangent, bitangent, normal);

	return normalize(mul(tangentSpaceNormal, TBN));
}

// Normalize that tolerates zero-length input (e.g. degenerate UV derivatives)
float3 SafeNormalize(float3 v)
{
	return v * rsqrt(max(dot(v, v), 1e-12f));
}

// Tilts the normal by the material's grayscale height map, if it has one.
// The surface directions of +U/+V come from screen-space derivatives of position and UV, so this
// needs no vertex tangents and is correct regardless of coordinate-system handedness or UV flips.
float3 ApplyHeightMap(float3 normal, float3 worldPos, float2 uv)
{
	if (HasHeightMap < 0.5f)
	{
		return normal;
	}

	float3 dpdx = ddx(worldPos);
	float3 dpdy = ddy(worldPos);
	float2 duvdx = ddx(uv);
	float2 duvdy = ddy(uv);

	float3 r1 = cross(dpdy, normal);
	float3 r2 = cross(normal, dpdx);
	float det = dot(dpdx, r1);

	float3 dirU = SafeNormalize(sign(det) * (r1 * duvdx.x + r2 * duvdy.x));
	float3 dirV = SafeNormalize(sign(det) * (r1 * duvdx.y + r2 * duvdy.y));

	float2 texSize;
	HeightMap.GetDimensions(texSize.x, texSize.y);
	float2 texel = 1.0f / texSize;

	float h  = HeightMap.Sample(LinearSampler, uv).r;
	float hU = HeightMap.Sample(LinearSampler, uv + float2(texel.x, 0.0f)).r;
	float hV = HeightMap.Sample(LinearSampler, uv + float2(0.0f, texel.y)).r;

	float3 heightGradient = (hU - h) * dirU + (hV - h) * dirV;
	return normalize(normal - BumpStrength * heightGradient);
}

//
// Shadows
//
// Fraction of the directional light reaching worldPos: 1 = fully lit, 0 = fully shadowed
float SampleShadow(float3 worldPos, float3 vertexNormal)
{
	if (HasShadows < 0.5f)
	{
		return 1.0f;
	}

	// Push the lookup off the surface so it doesn't shadow itself (acne)
	float3 offsetPos = worldPos + vertexNormal * ShadowNormalOffset;
	float4 lightClip = mul(LightViewProjection, float4(offsetPos, 1.0f));
	float3 lightNDC = lightClip.xyz / lightClip.w;

	float2 uv = float2(lightNDC.x * 0.5f + 0.5f, -lightNDC.y * 0.5f + 0.5f);

	// 4x4 grid of hardware-filtered (bilinear 2x2) comparisons: a smooth ~5x5 texel penumbra
	float lit = 0.0f;
	[unroll]
	for (int y = 0; y < 4; ++y)
	{
		[unroll]
		for (int x = 0; x < 4; ++x)
		{
			float2 offset = (float2(x, y) - 1.5f) * ShadowTexelSize;
			lit += ShadowMap.SampleCmpLevelZero(ShadowSampler, uv + offset, lightNDC.z);
		}
	}
	return lit / 16.0f;
}

//
// Lighting
//
float3 HemisphereAmbient(float3 normal)
{
	return lerp(AmbientGround, AmbientSky, normal.y * 0.5f + 0.5f);
}

// Schlick's Fresnel approximation: reflectance rises from f0 to f90 at glancing angles
float FresnelSchlick(float f0, float f90, float VdotH)
{
	return f0 + (f90 - f0) * pow(1.0f - VdotH, 5.0f);
}

// Energy-normalized Blinn-Phong lobe: tighter highlights get brighter instead of just smaller.
// (n + 8) / 8 rather than / 8pi because the diffuse term here also omits the 1/pi.
float BlinnPhongSpecular(float NdotH, float shininess)
{
	return (shininess + 8.0f) / 8.0f * pow(NdotH, shininess);
}

// Returns linear HDR radiance for the scene's directional light
float3 ComputeLighting(SurfaceData surface, float3 viewDir)
{
	float3 N = surface.Normal;
	float3 L = -LightDirection;
	float3 H = normalize(L + viewDir);

	float NdotL = saturate(dot(N, L));
	float NdotH = saturate(dot(N, H));
	float VdotH = saturate(dot(viewDir, H));

	float3 radiance = SRGBToLinear(LightColour.rgb) * LightStrength;

	// The specular mask also scales f90 so unmasked areas don't pick up a glancing-angle sheen
	float fresnel = FresnelSchlick(surface.SpecularIntensity * SpecularF0Scale, surface.SpecularIntensity, VdotH);
	float shininess = max(surface.Shininess, 1.0f);

	float3 diffuse = surface.Albedo * NdotL;
	float3 specular = fresnel * BlinnPhongSpecular(NdotH, shininess) * NdotL;
	float3 ambient = HemisphereAmbient(N) * surface.Albedo;

	float shadow = SampleShadow(surface.WorldPos, surface.VertexNormal);

	return ambient + (diffuse + specular) * radiance * shadow;
}

#endif // LIGHTING_HLSLI
