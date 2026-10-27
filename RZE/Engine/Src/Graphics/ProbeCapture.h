#pragma once

#include <EngineCore/Resources/TypedResourceHandle.h>

#include <Rendering/BufferHandle.h>

#include <Utils/Math/Vector3D.h>
#include <Utils/PrimitiveDefs.h>

class PixelShader;
class RenderScene;
class VertexShader;

// Draws what a probe sees, for relighting later with whatever the lights are then: through six square faces, side by
// side, the nearest surface each texel sees (albedo, normal, distance, and whether it's a back face) packed into an
// RG32_UINT texel (Pixel_ProbeCapture.hlsl, Common/Probes.hlsli). Texels that see nothing stay as cleared (0): the sky.
//
// The faces are plain look-at views, so back faces show as back faces (SV_IsFrontFace), in one of two layouts:
// EProbeFaces::Probe's own (GetFaceBasis), read texel by texel, or CubeMap's, whose faces are a cube texture's slices,
// so a capture's texels are a cube's texels (reflection probes).
class ProbeCapture
{
public:
	static constexpr U32 k_faceCount = 6;

	enum class EProbeFaces
	{
		// GetFaceBasis; irradiance probes
		Probe,
		// CubeMap::GetFaceForward and GetFaceUp, in ECubeFace order; reflection probes
		CubeMap
	};

	// Mirrors ProbeCaptureBuffer in Pixel_ProbeCapture.hlsl
	struct ParamsLayout
	{
		float ProbePosition[3];
		float _pad;
	};

	// The direction a face looks along, and its up. Mirrors ProbeFaceForward and ProbeFaceUp (Common/Probes.hlsli).
	struct FaceBasis
	{
		Vector3D Forward;
		Vector3D Up;
	};

	static FaceBasis GetFaceBasis(U32 face);
	// The unit direction from the probe through the centre of texel (x, y) of a face, from its top-left. Mirrors
	// ProbeTexelDirection (Common/Probes.hlsli).
	static Vector3D GetTexelDirection(U32 face, U32 x, U32 y, U32 faceSize);

public:
	// faceSize: texels across each face
	ProbeCapture(U32 faceSize, EProbeFaces faces);

	void Initialize();

	// Draws the probe at position's faces into the bound colour and depth targets, side by side from (left, top),
	// faceSize * k_faceCount wide. Leaves out Movable objects, what's further than farDistance, blended draws, and
	// draws smaller than a texel. Returns how many draws it made.
	U32 Capture(const RenderScene& scene, const Vector3D& position, float farDistance, U32 left, U32 top);

	U32 GetFaceSize() const { return m_faceSize; }

private:
	U32 m_faceSize;
	EProbeFaces m_faces;

	// The forward pass's, so surfaces are where it draws them
	TypedResourceHandle<VertexShader> m_vertexShader;
	TypedResourceHandle<PixelShader> m_pixelShader;
	// Its own, so the forward pass's camera isn't overwritten
	Rendering::ConstantBufferHandle m_cameraBuffer;
	Rendering::ConstantBufferHandle m_paramsBuffer;
};
