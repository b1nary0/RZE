#pragma once

#include <EngineCore/Resources/TypedResourceHandle.h>

#include <Rendering/BufferHandle.h>

#include <Utils/Math/Matrix4x4.h>
#include <Utils/Math/Vector3D.h>
#include <Utils/PrimitiveDefs.h>

#include <memory>

class BoundingBox;
class PixelShader;
class RenderScene;
class VertexShader;

// Draws what probes see, for relighting later with whatever the lights are then: through six square faces each, side by
// side, the nearest surface each texel sees (albedo, normal, distance, and whether it's a back face) packed into an
// RG32_UINT texel (Pixel_ProbeCapture.hlsl, Common/Probes.hlsli). Texels that see nothing stay as cleared (0): the sky.
//
// Probes are captured in batches: each draw is drawn once for the whole batch, instanced once for each face that sees it
// (Vertex_ProbeCapture.hlsl), rather than once for each face.
//
// The faces are plain look-at views, so back faces show as back faces (SV_IsFrontFace), in one of two layouts:
// EProbeFaces::Probe's own (GetFaceBasis), read texel by texel, or CubeMap's, whose faces are a cube texture's slices,
// so a capture's texels are a cube's texels (reflection probes).
class ProbeCapture
{
public:
	static constexpr U32 k_faceCount = 6;
	// Every face, as a face mask (a bit per face, in the layout's order)
	static constexpr U32 k_allFaces = (1u << k_faceCount) - 1;
	// The most faces one batch draws, 64 probes' worth; mirrors MAX_CAPTURE_FACES in Vertex_ProbeCapture.hlsl
	static constexpr U32 k_maxBatchFaces = 384;

	enum class EProbeFaces
	{
		// GetFaceBasis; irradiance probes
		Probe,
		// CubeMap::GetFaceForward and GetFaceUp, in ECubeFace order; reflection probes
		CubeMap
	};

	// Mirrors CaptureFace in Vertex_ProbeCapture.hlsl: one face of a probe in the batch
	struct FaceLayout
	{
		Matrix4x4 ViewProjection;
		// Where the face is in the target, in its clip space: x and y scale, then x and y offset
		float Placement[4];
		// xyz: the probe's
		float ProbePosition[4];
	};

	// Mirrors CaptureFaces in Vertex_ProbeCapture.hlsl
	struct FaceTableLayout
	{
		FaceLayout Faces[k_maxBatchFaces];
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
	~ProbeCapture();

	void Initialize();

	// Which faces of a probe at position (in this capture's layout) might see something in box: a face mask. Every face
	// for a box around the probe.
	U32 GetFacesSeeing(const Vector3D& position, const BoundingBox& box) const;

	// Starts a batch, to be drawn into colour and depth targets targetWidth x targetHeight texels
	void BeginBatch(U32 targetWidth, U32 targetHeight);
	// Adds the faces in faceMask of the probe at position to the batch, its faces side by side from (left, top),
	// faceSize * k_faceCount wide, each in its own place whether it's in faceMask or not. False, adding none, if the
	// batch hasn't room for them all (k_maxBatchFaces).
	bool AddProbe(const Vector3D& position, float farDistance, U32 left, U32 top, U32 faceMask = k_allFaces);
	// Draws the batch's faces into the bound colour and depth targets, then ends the batch. Leaves out Movable objects,
	// blended draws, draws smaller than a texel from a probe, and draws no face sees. Returns how many draws it made:
	// one for each draw any face sees.
	U32 Flush(const RenderScene& scene);

	U32 GetFaceSize() const { return m_faceSize; }

private:
	FaceBasis GetFaceBasisOf(U32 face) const;

private:
	// The faces added since BeginBatch, and where each draw's instances go. Only while a batch is open.
	struct Batch;

	U32 m_faceSize;
	EProbeFaces m_faces;

	std::unique_ptr<Batch> m_batch;

	TypedResourceHandle<VertexShader> m_vertexShader;
	TypedResourceHandle<PixelShader> m_pixelShader;
	// FaceTableLayout
	Rendering::ConstantBufferHandle m_faceTableBuffer;
	// Which face each instance is drawn into (a U32 each), grown when a batch needs more
	Rendering::VertexBufferHandle m_instanceBuffer;
	size_t m_instanceCapacity = 0;
};
