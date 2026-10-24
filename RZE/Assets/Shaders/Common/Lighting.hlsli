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
// Linear-space hemisphere ambient: sky colour for up-facing normals, ground colour for down-facing.
// Kept well below the light strength (~2) so lit and unlit sides read clearly. Mirrored by k_ambientSky and
// k_ambientGround (RenderScene.cpp) for the editor's point light gizmo.
static const float3 AmbientSky    = float3(0.14f, 0.16f, 0.20f);
static const float3 AmbientGround = float3(0.05f, 0.045f, 0.04f);
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
// Point light falloff: inverse square over distance as a fraction of the range, so the range sets how far the light
// reaches. Higher concentrates it nearer the light: at half the range it's 1 / (1 + this / 4) of its strength,
// before the window that takes it to 0 at the range. Mirrors k_pointFalloffSharpness (RenderScene.cpp), which the
// editor's range gizmo uses.
static const float  PointFalloffSharpness = 8.0f;

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
// Shadows
//
// Fraction of the directional light reaching worldPos: 1 = fully lit, 0 = fully shadowed
float SampleDirectionalShadow(float3 worldPos, float3 vertexNormal)
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

// The direction to sample a cube texture with to read what was rendered towards worldDirection. D3D's cube is
// left-handed with +Z forward and ours is right-handed with -Z forward, so Z flips. Mirrors
// CubeMap::ToSampleDirection (Utils/Math/CubeMap.h).
float3 ToCubeSampleDirection(float3 worldDirection)
{
	return float3(worldDirection.xy, -worldDirection.z);
}

// Fraction of a shadowed point light reaching worldPos: 1 = fully lit, 0 = fully shadowed
float SamplePointShadow(Light light, float3 worldPos, float3 vertexNormal)
{
	PointShadow shadow = PointShadows[light.ShadowIndex];

	// Push the lookup off the surface so it doesn't shadow itself (acne), by more further out, where texels are bigger
	float distanceToLight = length(worldPos - light.Position);
	float3 fromLight = worldPos + vertexNormal * (shadow.NormalOffsetPerMetre * distanceToLight) - light.Position;

	// Each face stores depth by distance along its own forward, which for the face a direction falls in is the
	// direction's largest component
	float3 absFromLight = abs(fromLight);
	float faceDistance = max(absFromLight.x, max(absFromLight.y, absFromLight.z));
	float depth = shadow.DepthAtInfinity + shadow.DepthDistanceScale / faceDistance;

	// Four hardware-filtered (bilinear 2x2) comparisons around the direction, a texel and a half out across the face,
	// for a soft edge. The cube's faces join seamlessly, so taps can cross onto the next face.
	float3 direction = fromLight / faceDistance;
	float3 side = normalize(cross(direction, abs(direction.y) < 0.99f ? float3(0.0f, 1.0f, 0.0f) : float3(1.0f, 0.0f, 0.0f)));
	float3 up = cross(side, normalize(direction));
	// A face is 2 across at a distance of 1 along its forward
	float tapOffset = 1.5f * 2.0f * PointShadowTexelSize;

	float lit = 0.0f;
	[unroll]
	for (int tap = 0; tap < 4; ++tap)
	{
		float2 offset = float2(tap & 1 ? 1.0f : -1.0f, tap & 2 ? 1.0f : -1.0f) * tapOffset;
		float3 tapDirection = direction + side * offset.x + up * offset.y;
		lit += PointShadowCubes.SampleCmpLevelZero(ShadowSampler, float4(ToCubeSampleDirection(tapDirection), light.ShadowIndex), depth);
	}
	return lit / 4.0f;
}

//
// Lighting
//
float3 LightRadiance(Light light)
{
	return SRGBToLinear(light.Colour) * light.Strength;
}

// How much of the light reaches worldPos, before shadows, and the direction towards the light (outL). The light's
// type decides both.
float LightAttenuation(Light light, float3 worldPos, out float3 outL)
{
	if (light.Type == LIGHT_TYPE_POINT)
	{
		float3 toLight = light.Position - worldPos;
		float distanceSq = dot(toLight, toLight);
		outL = toLight * rsqrt(max(distanceSq, 1e-8f));

		// Inverse square in fractions of the range rather than metres, so a longer range reaches further instead of
		// only moving where a physically faded light is cut off. Windowed so it reaches exactly 0 at the range, and
		// the 1 + keeps it finite at the light, so Strength is the brightness right next to it.
		// Mirrors LightObject::CalculatePointFalloff.
		float rangeRatioSq = distanceSq / max(light.Range * light.Range, 1e-8f);
		float window = saturate(1.0f - rangeRatioSq * rangeRatioSq);
		return window * window / (1.0f + PointFalloffSharpness * rangeRatioSq);
	}

	outL = -light.Direction;
	return 1.0f;
}

// Sky above, ground below, or the same all round with uniform lighting. Also used as the environment for ambient
// reflections.
float3 HemisphereAmbient(float3 direction)
{
	if (UniformLighting > 0.5f)
	{
		return UniformAmbient;
	}
	return lerp(AmbientGround, AmbientSky, direction.y * 0.5f + 0.5f);
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

	// Stand-in for specular occlusion: nothing blocks the hemisphere, so a broad, rough lobe would
	// otherwise put a grey sheen over every crevice and wash out coloured materials like rust
	float reflectionWeight = gloss * gloss;

	// Light that reflects off the surface isn't also scattered as diffuse
	float3 diffuse = HemisphereAmbient(N) * surface.Albedo * (1.0f - fresnel);
	float3 reflection = environment * fresnel * reflectionWeight;

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

// Fraction of the light reaching the surface past whatever casts shadows: 1 for a light without a shadow map
float LightShadow(Light light, SurfaceData surface)
{
	// One return at the end: FXC warns of an uninitialized return value (X4000) for an early one here, since
	// the sampling loops
	float shadow = 1.0f;
	// Each type has its own kind of shadow map
	if (light.ShadowIndex >= 0)
	{
		if (light.Type == LIGHT_TYPE_POINT)
		{
			shadow = SamplePointShadow(light, surface.WorldPos, surface.VertexNormal);
		}
		else
		{
			shadow = SampleDirectionalShadow(surface.WorldPos, surface.VertexNormal);
		}
	}
	return shadow;
}

// Returns linear HDR radiance for the scene's lights, or for the uniform environment alone
float3 ComputeLighting(SurfaceData surface, float3 viewDir)
{
	float shininess = max(surface.Shininess, 1.0f);
	float3 colour = AmbientLighting(surface, viewDir, shininess);

	// One return at the end: FXC warns of an uninitialized return value (X4000) for an early one before the loop
	if (UniformLighting > 0.5f)
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

			colour += DirectLighting(surface, viewDir, L, shininess) * LightRadiance(light) * attenuation * LightShadow(light, surface);
		}
	}

	return colour;
}

#endif // LIGHTING_HLSLI
