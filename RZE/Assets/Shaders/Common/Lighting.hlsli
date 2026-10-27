#ifndef LIGHTING_HLSLI
#define LIGHTING_HLSLI

// Include paths are relative to Assets/Shaders/: the engine's D3D_COMPILE_STANDARD_FILE_INCLUDE
// resolves nested includes from the top-level shader's directory, not the including file's.
#include "Common/ColourSpace.hlsli"
#include "Common/PixelResources.hlsli"

//
// Tunables
//
static const float  AlphaCutoff  = 0.5f;
// Height-map bump: normal tilt per unit of height change per texel
static const float  BumpStrength = 10.0f;
// Linear-space environment for uniform lighting, the same from every direction: a surface shows its albedo, as it
// would facing a strength-1 light. Auto-exposure sets the final brightness.
static const float3 UniformAmbient = float3(1.0f, 1.0f, 1.0f);
// Uniform lighting alone gives every face of a plain mesh the same colour, so it reads as a silhouette. Faces are
// shaded from 1 facing the camera down to this edge-on, as if lit from the camera, so the shape still shows.
static const float  UniformEdgeShade = 0.45f;
// Blinn-Phong exponent treated as a mirror for ambient reflections; log2(2048) = 11
static const float  MaxShininessLog2 = 11.0f;
// Specular reflectance at normal incidence = SpecularIntensity * this; 0.5 intensity gives the typical dielectric 0.04
static const float  SpecularF0Scale = 0.08f;

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
	float  AmbientOcclusion;  // 0..1 of the indirect light reaching the surface past nearby geometry; SampleAmbientOcclusion
};

// The view's ambient occlusion at a pixel (SV_Position.xy). Clamped to the texture, so the 1x1 white one of a view
// without ambient occlusion reads 1 everywhere.
float SampleAmbientOcclusion(float2 pixelPosition)
{
	uint2 size;
	AmbientOcclusionMap.GetDimensions(size.x, size.y);
	return AmbientOcclusionMap.Load(int3(min(uint2(pixelPosition), size - 1), 0));
}

// How much of the pixel the material's opacity mask covers, 1 without a mask. A cutout material discards the pixel
// below AlphaCutoff instead (and returns 1); a blended one returns the mask, to fade by. Diffuse alpha isn't used
// unless the material says so, because some assets (e.g. Nanosuit) store unrelated data there.
// The surface's linear colour: the diffuse texture's, tinted, or the tint alone without one
float3 GetAlbedo(float2 uv)
{
	const float3 tint = SRGBToLinear(Tint);
	if (HasDiffuseMap < 0.5f)
	{
		return tint;
	}
	return SRGBToLinear(DiffuseMap.Sample(LinearSampler, uv).rgb) * tint;
}

float ApplyOpacityMask(float2 uv)
{
	if (HasOpacityMask < 0.5f)
	{
		return 1.0f;
	}

	float4 maskSample = OpacityMap.Sample(LinearSampler, uv);
	float mask = OpacityMaskInAlpha > 0.5f ? maskSample.a : maskSample.r;
	if (InvertOpacityMask > 0.5f)
	{
		mask = 1.0f - mask;
	}

	if (OpacityMaskIsCoverage > 0.5f)
	{
		return mask;
	}

	clip(mask - AlphaCutoff);
	return 1.0f;
}

// Applies a tangent-space normal map sample to the world-space vertex normal
float3 PerturbNormal(float3 normal, float3 tangent, float3 normalMapSample)
{
	// Gram-Schmidt: re-orthogonalize the interpolated tangent against the normal
	tangent = normalize(tangent - dot(tangent, normal) * normal);
	float3 bitangent = cross(normal, tangent);

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
// Lighting
//
// Sky above, ground below, or the same all round with uniform lighting. Also used as the environment for ambient
// reflections.
float3 HemisphereAmbient(float3 direction)
{
	if (UniformLighting > 0.5f)
	{
		return UniformAmbient;
	}
	return SkyRadiance(direction);
}

// The light arriving indirectly at a surface facing normal, as it would light a white diffuse surface: from the
// irradiance probes where they've been lit, otherwise from the hemisphere
float3 IndirectDiffuse(float3 worldPos, float3 normal)
{
	float3 irradiance;
	if (UniformLighting > 0.5f || !SampleProbeIrradiance(worldPos, normal, irradiance))
	{
		return HemisphereAmbient(normal);
	}
	return irradiance;
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

// Ambient occlusion counts light blocked on its way in, but some of it bounces off the occluding surfaces back in. A fit of
// how much, by albedo (Jimenez et al. 2016, "Practical Real-Time Strategies for Accurate Indirect Occlusion"), so bright
// surfaces don't go grey in their creases. Capped at 1: the fit is a hair over it where nothing occludes.
float3 MultiBounceAmbientOcclusion(float visibility, float3 albedo)
{
	const float3 a = 2.0404f * albedo - 0.3324f;
	const float3 b = -4.7951f * albedo + 0.6417f;
	const float3 c = 2.7552f * albedo + 0.6903f;
	return min(max(visibility, ((visibility * a + b) * visibility + c) * visibility), 1.0f);
}

// How much of a reflection the ambient occlusion blocks: little looking straight at a surface, more at glancing angles
// and for glossier ones, whose reflection comes from a narrower cone (Lagarde and de Rousiers 2014, "Moving Frostbite to
// PBR"). roughness is 0 (mirror) to 1.
float SpecularOcclusion(float NdotV, float visibility, float roughness)
{
	// The base is never negative; the max tells the compiler so
	return saturate(pow(max(NdotV + visibility, 0.0f), exp2(-16.0f * roughness - 1.0f)) - 1.0f + visibility);
}

// Hemisphere ambient, diffuse plus reflection. Without the reflection, glossy dark materials in shade
// (black armour, iron) render as albedo x ambient, which is near black, when they'd really mirror their surroundings.
float3 AmbientLighting(SurfaceData surface, float3 viewDir, float shininess)
{
	float3 N = surface.Normal;
	float NdotV = saturate(dot(N, viewDir));

	// 0 = rough, 1 = mirror. Rough surfaces see a blurred environment and lose the glancing-angle boost.
	float gloss = saturate(log2(shininess) / MaxShininessLog2);

	float3 R = reflect(-viewDir, N);
	float3 environment = HemisphereAmbient(normalize(lerp(N, R, gloss)));

	float f0 = surface.SpecularIntensity * SpecularF0Scale;
	float fresnel = FresnelSchlick(f0, lerp(f0, surface.SpecularIntensity, gloss), NdotV);

	// Stand-in for occlusion at a larger scale than the ambient occlusion sees (about half a metre): nothing blocks the
	// hemisphere, so a broad, rough lobe would otherwise put a grey sheen over every crevice and wash out coloured
	// materials like rust. To go once probes occlude the sky (Docs/Plans/SSAOAndGlobalIllumination.md).
	float reflectionWeight = gloss * gloss;

	// Indirect light only: direct lights have their shadows
	float3 diffuseOcclusion = MultiBounceAmbientOcclusion(surface.AmbientOcclusion, surface.Albedo);
	float specularOcclusion = SpecularOcclusion(NdotV, surface.AmbientOcclusion, 1.0f - gloss);

	// Light that reflects off the surface isn't also scattered as diffuse
	float3 diffuse = IndirectDiffuse(surface.WorldPos, N) * surface.Albedo * (1.0f - fresnel) * diffuseOcclusion;
	float3 reflection = environment * fresnel * reflectionWeight * specularOcclusion;

	return diffuse + reflection;
}

// Diffuse plus specular reflected towards viewDir from a light arriving from direction L, per unit of radiance.
// Shared by every light type.
float3 DirectLighting(SurfaceData surface, float3 viewDir, float3 L, float shininess)
{
	float3 N = surface.Normal;
	float3 H = normalize(L + viewDir);

	float NdotL = saturate(dot(N, L));
	float NdotH = saturate(dot(N, H));
	float VdotH = saturate(dot(viewDir, H));

	// The specular mask also scales f90 so unmasked areas don't pick up a glancing-angle sheen
	float fresnel = FresnelSchlick(surface.SpecularIntensity * SpecularF0Scale, surface.SpecularIntensity, VdotH);

	float3 diffuse = surface.Albedo * NdotL;
	float3 specular = fresnel * BlinnPhongSpecular(NdotH, shininess) * NdotL;
	return diffuse + specular;
}

// Returns linear HDR radiance for the scene's lights, or for the uniform environment alone
float3 ComputeLighting(SurfaceData surface, float3 viewDir)
{
	float shininess = max(surface.Shininess, 1.0f);
	float3 colour = AmbientLighting(surface, viewDir, shininess);

	// One return at the end: FXC warns of an uninitialized return value (X4000) for an early one before the loop
	if (ShowIndirectDiffuse > 0.5f)
	{
		// ERenderViewDebug::IndirectDiffuse: what the ambient light alone would do to a white surface, occlusion included
		colour = IndirectDiffuse(surface.WorldPos, surface.Normal) * MultiBounceAmbientOcclusion(surface.AmbientOcclusion, 1.0f);
	}
	else if (UniformLighting > 0.5f)
	{
		float facing = saturate(dot(surface.Normal, viewDir));
		colour *= lerp(UniformEdgeShade, 1.0f, facing);
	}
	else
	{
		for (uint lightIndex = 0; lightIndex < LightCount; ++lightIndex)
		{
			Light light = Lights[lightIndex];

			float3 L;
			float attenuation = LightAttenuation(light, surface.WorldPos, L);
			if (attenuation <= 0.0f)
			{
				continue;
			}

			colour += DirectLighting(surface, viewDir, L, shininess) * LightRadiance(light) * attenuation * LightShadow(light, surface.WorldPos, surface.VertexNormal);
		}
	}

	return colour;
}

#endif // LIGHTING_HLSLI
