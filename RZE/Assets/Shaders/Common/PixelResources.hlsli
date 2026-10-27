#ifndef PIXEL_RESOURCES_HLSLI
#define PIXEL_RESOURCES_HLSLI

// The lights, shadow maps and sky (b2, b3, t5, t6, s1)
#include "Common/LightResources.hlsli"
// The irradiance probes (b5, t8 to t11)
#include "Common/IrradianceVolume.hlsli"
// The reflection probes (b7, t12)
#include "Common/ReflectionProbes.hlsli"

// Mirrors GraphicsLayouts::MaterialBufferLayout (Graphics/GraphicsLayouts.h)
cbuffer MaterialBuffer : register(b1)
{
	float Shininess;
	float Opacity;
	float HasOpacityMask; // 1 when OpacityMap is bound for this material
	float HasHeightMap;   // 1 when HeightMap is bound for this material
	float OpacityMaskInAlpha; // 1 when the cutout is in OpacityMap's alpha (map_d == map_Kd) instead of red
	float InvertOpacityMask;  // 1 when the mask's white means see-through
	float OpacityMaskIsCoverage; // 1 for blended materials: the mask fades rather than cuts
	float HasDiffuseMap; // 1 when DiffuseMap is bound for this material; otherwise Tint alone is its colour
	float3 Tint; // sRGB; multiplies DiffuseMap, or is the colour without it
	float HasSpecularMap; // 1 when SpecularMap is bound for this material
};

// Mirrors GraphicsLayouts::ViewLightingLayout (Graphics/GraphicsLayouts.h)
cbuffer ViewLightingBuffer : register(b4)
{
	float UniformLighting; // 1 to light evenly from every direction instead of by the light and sky (ERenderViewLighting)
	float ShowIndirectDiffuse; // 1 to show the indirect light alone, as it would light a white surface (ERenderViewDebug)
};

// Mirrors MaterialInstance::TextureSlot (Material.h)
Texture2D DiffuseMap  : register(t0);
Texture2D SpecularMap : register(t1);
Texture2D NormalMap   : register(t2);
Texture2D OpacityMap  : register(t3); // Cutout mask in .r (or .a, see OpacityMaskInAlpha); only valid when HasOpacityMask is set
Texture2D HeightMap   : register(t4); // Grayscale height in .r; only valid when HasHeightMap is set

// t5, t6: the shadow maps (LightResources.hlsli)
// The view's ambient occlusion, 1 = nothing occludes (AmbientOcclusionStage); read with SampleAmbientOcclusion (Lighting.hlsli)
Texture2D<float> AmbientOcclusionMap : register(t7); // Bound by ForwardRenderStage

SamplerState LinearSampler : register(s0);

#endif // PIXEL_RESOURCES_HLSLI
