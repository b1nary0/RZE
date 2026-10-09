#ifndef PIXEL_RESOURCES_HLSLI
#define PIXEL_RESOURCES_HLSLI

// Mirrors MaterialInstance::MaterialParams (Material.h)
cbuffer MaterialBuffer : register(b1)
{
	float Shininess;
	float Opacity;
	float HasOpacityMask; // 1 when OpacityMap is bound for this material
	float HasHeightMap;   // 1 when HeightMap is bound for this material
	float OpacityMaskInAlpha; // 1 when the cutout is in OpacityMap's alpha (map_d == map_Kd) instead of red
};

// Mirrors LightObject::PropertyBufferLayout (RenderEngine.h)
cbuffer LightBuffer : register(b2)
{
	float3 LightDirection; // World space, normalized; the direction light travels
	float4 LightColour;
	float LightStrength;
};

// Mirrors ShadowRenderStage::ShadowBufferLayout (ShadowRenderStage.h)
cbuffer ShadowBuffer : register(b3)
{
	float4x4 LightViewProjection;
	float ShadowTexelSize;    // 1 / shadow map resolution
	float ShadowNormalOffset; // World units to push the lookup along the surface normal
	float HasShadows;         // 0 when there is no light to cast from
};

// Mirrors MaterialInstance::TextureSlot (Material.h)
Texture2D DiffuseMap  : register(t0);
Texture2D SpecularMap : register(t1);
Texture2D NormalMap   : register(t2);
Texture2D OpacityMap  : register(t3); // Cutout mask in .r (or .a, see OpacityMaskInAlpha); only valid when HasOpacityMask is set
Texture2D HeightMap   : register(t4); // Grayscale height in .r; only valid when HasHeightMap is set

Texture2D ShadowMap   : register(t5); // Bound by ForwardRenderStage

SamplerState LinearSampler : register(s0);
SamplerComparisonState ShadowSampler : register(s1); // LESS_EQUAL compare, border = lit (DX11Device)

#endif // PIXEL_RESOURCES_HLSLI
