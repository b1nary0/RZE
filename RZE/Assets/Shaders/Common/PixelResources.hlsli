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
	float InvertOpacityMask;  // 1 when the mask's white means see-through
	float OpacityMaskIsCoverage; // 1 for blended materials: the mask fades rather than cuts
	float HasDiffuseMap; // 1 when DiffuseMap is bound for this material; otherwise Tint alone is its colour
	float3 Tint; // sRGB; multiplies DiffuseMap, or is the colour without it
	float HasSpecularMap; // 1 when SpecularMap is bound for this material
};

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
	// Without it, LightCount would pack into the last light's spare bytes instead of where the C++ layout has it
	float3 _Pad;
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

// Mirrors GraphicsLayouts::ViewLightingLayout (Graphics/GraphicsLayouts.h)
cbuffer ViewLightingBuffer : register(b4)
{
	float UniformLighting; // 1 to light evenly from every direction instead of by the light and sky (ERenderViewLighting)
};

// Mirrors MaterialInstance::TextureSlot (Material.h)
Texture2D DiffuseMap  : register(t0);
Texture2D SpecularMap : register(t1);
Texture2D NormalMap   : register(t2);
Texture2D OpacityMap  : register(t3); // Cutout mask in .r (or .a, see OpacityMaskInAlpha); only valid when HasOpacityMask is set
Texture2D HeightMap   : register(t4); // Grayscale height in .r; only valid when HasHeightMap is set

Texture2D ShadowMap   : register(t5); // Bound by ForwardRenderStage
// A cube per shadowed point light, by its ShadowIndex; sample with ToCubeSampleDirection (Lighting.hlsli)
TextureCubeArray PointShadowCubes : register(t6); // Bound by ForwardRenderStage

SamplerState LinearSampler : register(s0);
SamplerComparisonState ShadowSampler : register(s1); // LESS_EQUAL compare, border = lit (DX11Device)

#endif // PIXEL_RESOURCES_HLSLI
