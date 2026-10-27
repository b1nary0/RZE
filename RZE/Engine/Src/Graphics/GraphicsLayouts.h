#pragma once

#include <Utils/PrimitiveDefs.h>
#include <Utils/Math/Matrix4x4.h>
#include <Utils/Math/Vector3D.h>

// The CPU side of shader buffers that aren't one render stage's own, such as the scene's lights, which any stage or
// shader may read. A layout only one stage's shaders use (e.g. PickRenderStage::PickBufferLayout) stays with that
// stage.
namespace GraphicsLayouts
{
	// Mirrors MAX_LIGHTS in Common/PixelResources.hlsli. Lights past it are left out.
	constexpr U32 k_maxLights = 16;
	// Mirrors MAX_SHADOWED_POINT_LIGHTS in Common/PixelResources.hlsli. Point lights past it light without shadows.
	constexpr U32 k_maxShadowedPointLights = 8;

	// Mirrors Light in Common/PixelResources.hlsli. Each type reads only what it uses (LightObject).
	struct LightLayout
	{
		Vector3D Position;
		float Range = 0.0f;
		Vector3D Direction;
		U32 Type = 0; // ELightType
		Vector3D Colour; // sRGB
		float Strength = 0.0f;
		Int32 ShadowIndex = -1; // Which of its type's shadow maps it casts into; -1 for none
		float _pad[3] = {}; // A cbuffer array element is a whole number of 16-byte registers
	};

	// Mirrors LightBuffer in Common/PixelResources.hlsli
	struct LightBufferLayout
	{
		LightLayout Lights[k_maxLights];
		U32 Count = 0;
		float _pad[3] = {};
	};

	// Mirrors PointShadow in Common/PixelResources.hlsli: how to read one point light's shadow cube
	struct PointShadowLayout
	{
		// The depth stored for something distance metres along a face's forward is
		// DepthAtInfinity + DepthDistanceScale / distance: the faces' projection, worked out from its matrix
		float DepthAtInfinity = 0.0f;
		float DepthDistanceScale = 0.0f;
		// World units to push the lookup along the surface normal, per metre from the light, since the faces' texels
		// grow with distance
		float NormalOffsetPerMetre = 0.0f;
		float _pad = 0.0f;
	};

	// Mirrors ShadowBuffer in Common/PixelResources.hlsli
	struct ShadowBufferLayout
	{
		Matrix4x4 LightViewProjection;
		float TexelSize;     // 1 / shadow map resolution
		float NormalOffset;  // World units to push the lookup along the surface normal
		float HasShadows;    // 0 when there is no light to cast from
		float PointTexelSize; // 1 / a point light's cube face resolution
		// By a point light's ShadowIndex
		PointShadowLayout PointShadows[k_maxShadowedPointLights];
	};

	// Mirrors ViewLightingBuffer in Common/PixelResources.hlsli
	struct ViewLightingLayout
	{
		float UniformLighting = 0.0f; // 1 for ERenderViewLighting::Uniform
		float _pad[3] = {}; // A cbuffer is a whole number of 16-byte registers
	};

	// Mirrors ObjectBuffer in Common/VertexResources.hlsli: a world transform and its inverse, side by side, uploaded
	// together. RenderObject keeps its own this way, and so does a MeshDraw placed away from its mesh's origin.
	struct ObjectBufferLayout
	{
		Matrix4x4 Transform;
		Matrix4x4 InvTransform;
	};

	// Mirrors MaterialBuffer in Common/PixelResources.hlsli. MaterialInstance::BuildBufferData works it out from a
	// material's values.
	// #TODO(Josh::Turn into material properties or something; material system?)
	struct MaterialBufferLayout
	{
		float Shininess{ 1.0f };
		float Opacity{ 1.0f };
		// 1.0f when the matching texture slot is set. Unset slots aren't unbound, so the shader needs
		// these to avoid sampling a previous draw's texture.
		float HasOpacityMask{ 0.0f };
		float HasHeightMap{ 0.0f };
		// 1.0f when the opacity mask is in the texture's alpha channel rather than red
		float OpacityMaskInAlpha{ 0.0f };
		// 1.0f when the mask's white means see-through rather than solid
		float InvertOpacityMask{ 0.0f };
		// 1.0f when the mask is how much of the material shows (blended materials) rather than where it's cut away
		float OpacityMaskIsCoverage{ 0.0f };
		// 1.0f when the diffuse slot is set; without it the tint alone is the colour. 1.0f by default, since materials
		// made in code set their textures themselves (SetTexture) and keep the defaults.
		float HasDiffuseMap{ 1.0f };
		// MaterialValues::Tint; HLSL starts a float3 on a new 16-byte register, as here
		float Tint[3]{ 1.0f, 1.0f, 1.0f };
		// 1.0f when the specular slot is set; without it the shader uses a typical non-metal's intensity. Fills the
		// rest of Tint's register, keeping the buffer a whole number of 16-byte registers, which is what's uploaded.
		float HasSpecularMap{ 0.0f };
	};
}
