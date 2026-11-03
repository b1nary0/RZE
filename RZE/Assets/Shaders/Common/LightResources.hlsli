#ifndef LIGHT_RESOURCES_HLSLI
#define LIGHT_RESOURCES_HLSLI

// The scene's lights, their shadow maps and the sky, and how much of each reaches a point: what the forward pass lights
// surfaces with, and what the probe relighting (Compute_ProbeRelight.hlsl) lights captured surfaces with. No material
// or view resources, so a compute shader can include it.

#include "Common/ColourSpace.hlsli"

// Mirrors GraphicsLayouts::k_maxLights (Graphics/GraphicsLayouts.h)
#define MAX_LIGHTS 16

// Mirrors ELightType (RenderScene.h)
#define LIGHT_TYPE_DIRECTIONAL 0
#define LIGHT_TYPE_POINT 1

// Mirrors GraphicsLayouts::LightLayout (Graphics/GraphicsLayouts.h). Each type reads only what it uses.
struct Light
{
	float3 Position;  // World space; point lights
	float  Range;     // Metres at which a point light has faded to nothing
	float3 Direction; // World space, normalized; the direction a directional light's light travels
	uint   Type;      // LIGHT_TYPE_*
	float3 Colour;    // sRGB
	float  Strength;
	int    ShadowIndex; // Which of its type's shadow maps it casts into; -1 for none
	// Without it, LightCount would pack into the last light's spare bytes instead of where the C++ layout has it. Three
	// scalars rather than a float3: GLSL can't place a vector at this offset, and the layout is the same.
	float _Pad0;
	float _Pad1;
	float _Pad2;
};

// Mirrors GraphicsLayouts::LightBufferLayout (Graphics/GraphicsLayouts.h)
cbuffer LightBuffer : register(b2)
{
	Light Lights[MAX_LIGHTS];
	uint  LightCount;
};

// Mirrors GraphicsLayouts::k_maxShadowedPointLights (Graphics/GraphicsLayouts.h)
#define MAX_SHADOWED_POINT_LIGHTS 8

// Mirrors GraphicsLayouts::PointShadowLayout (Graphics/GraphicsLayouts.h): how to read one point light's shadow cube
struct PointShadow
{
	// The depth stored for something this far along a cube face's forward is
	// DepthAtInfinity + DepthDistanceScale / distance
	float DepthAtInfinity;
	float DepthDistanceScale;
	float NormalOffsetPerMetre; // World units to push the lookup along the surface normal, per metre from the light
	float _Pad;
};

// Mirrors GraphicsLayouts::ShadowBufferLayout (Graphics/GraphicsLayouts.h)
cbuffer ShadowBuffer : register(b3)
{
	// The directional light's
	float4x4 LightViewProjection;
	float ShadowTexelSize;    // 1 / shadow map resolution
	float ShadowNormalOffset; // World units to push the lookup along the surface normal
	float HasShadows;         // 0 when there is no light to cast from
	float PointShadowTexelSize; // 1 / a point light's cube face resolution
	// By a point light's ShadowIndex
	PointShadow PointShadows[MAX_SHADOWED_POINT_LIGHTS];
};

Texture2D ShadowMap : register(t5); // Bound by ForwardRenderStage and IrradianceProbeStage
// A cube per shadowed point light, by its ShadowIndex; sample with ToCubeSampleDirection
TextureCubeArray PointShadowCubes : register(t6); // Bound by ForwardRenderStage and IrradianceProbeStage

SamplerComparisonState ShadowSampler : register(s1); // LESS_EQUAL compare, border = lit (DX11Device)
// Linear, clamped at the edges (DX11Device); for the probes' textures
SamplerState LinearClampSampler : register(s2);

// Linear-space hemisphere ambient: sky colour for up-facing normals, ground colour for down-facing.
// Kept well below the light strength (~2) so lit and unlit sides read clearly. Mirrored by k_ambientSky and
// k_ambientGround (RenderScene.cpp) for the editor's point light gizmo.
static const float3 AmbientSky    = float3(0.14f, 0.16f, 0.20f);
static const float3 AmbientGround = float3(0.05f, 0.045f, 0.04f);
// Point light falloff: inverse square over distance as a fraction of the range, so the range sets how far the light
// reaches. Higher concentrates it nearer the light: at half the range it's 1 / (1 + this / 4) of its strength,
// before the window that takes it to 0 at the range. Mirrors k_pointFalloffSharpness (RenderScene.cpp), which the
// editor's range gizmo uses.
static const float  PointFalloffSharpness = 8.0f;

// The light from the sky (and the ground beyond the scene) towards direction: sky above, ground below
float3 SkyRadiance(float3 direction)
{
	return lerp(AmbientGround, AmbientSky, direction.y * 0.5f + 0.5f);
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
// Lights
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

// Fraction of the light reaching worldPos past whatever casts shadows: 1 for a light without a shadow map. vertexNormal
// is the geometric normal there, which the lookups push off along.
float LightShadow(Light light, float3 worldPos, float3 vertexNormal)
{
	// One return at the end: FXC warns of an uninitialized return value (X4000) for an early one here, since
	// the sampling loops
	float shadow = 1.0f;
	// Each type has its own kind of shadow map
	if (light.ShadowIndex >= 0)
	{
		if (light.Type == LIGHT_TYPE_POINT)
		{
			shadow = SamplePointShadow(light, worldPos, vertexNormal);
		}
		else
		{
			shadow = SampleDirectionalShadow(worldPos, vertexNormal);
		}
	}
	return shadow;
}

#endif // LIGHT_RESOURCES_HLSLI
