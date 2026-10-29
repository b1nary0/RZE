#include <StdAfx.h>
#include <Graphics/ProbeCapture.h>

#include <Graphics/CubeMap.h>
#include <Graphics/IndexBuffer.h>
#include <Graphics/Material.h>
#include <Graphics/MaterialDefines.h>
#include <Graphics/MaterialSlot.h>
#include <Graphics/RenderEngine.h>
#include <Graphics/Shader.h>
#include <Graphics/Texture2D.h>
#include <Graphics/VertexBuffer.h>

#include <Rendering/Renderer.h>

#include <Utils/Math/BoundingBox.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace
{
	// Must match CaptureFaces's register in Vertex_ProbeCapture.hlsl
	constexpr U32 k_faceTableSlot = 2;
	// Close enough for a probe a few centimetres from a wall to still see it
	constexpr float k_nearDistance = 0.02f;
	// What the instance buffer starts with, in instances; it doubles when a batch needs more
	constexpr size_t k_initialInstanceCapacity = 16384;
}

struct ProbeCapture::Batch
{
	struct Probe
	{
		Vector3D Position;
		// By face, its place in the face table; -1 for a face not in the batch
		std::array<int, k_faceCount> FaceIndices;
	};

	// A draw any face sees, and its instances' range in Instances
	struct Draw
	{
		const MeshDraw* Draw;
		U32 FirstInstance;
		U32 InstanceCount;
	};

	U32 TargetWidth = 0;
	U32 TargetHeight = 0;
	std::vector<Probe> Probes;
	FaceTableLayout FaceTable;
	U32 FaceCount = 0;

	std::vector<U32> Instances;
	std::vector<Draw> Draws;
};

ProbeCapture::FaceBasis ProbeCapture::GetFaceBasis(U32 face)
{
	AssertExpr(face < k_faceCount);
	static const FaceBasis k_faces[k_faceCount] =
	{
		{ Vector3D(1.0f, 0.0f, 0.0f), Vector3D(0.0f, 1.0f, 0.0f) },
		{ Vector3D(-1.0f, 0.0f, 0.0f), Vector3D(0.0f, 1.0f, 0.0f) },
		{ Vector3D(0.0f, 1.0f, 0.0f), Vector3D(0.0f, 0.0f, -1.0f) },
		{ Vector3D(0.0f, -1.0f, 0.0f), Vector3D(0.0f, 0.0f, 1.0f) },
		{ Vector3D(0.0f, 0.0f, 1.0f), Vector3D(0.0f, 1.0f, 0.0f) },
		{ Vector3D(0.0f, 0.0f, -1.0f), Vector3D(0.0f, 1.0f, 0.0f) }
	};
	return k_faces[face];
}

Vector3D ProbeCapture::GetTexelDirection(U32 face, U32 x, U32 y, U32 faceSize)
{
	// As the face's view (Matrix4x4::CreateViewMatrix) sees it: right and up across the screen, 90 degrees across
	const FaceBasis basis = GetFaceBasis(face);
	const Vector3D right = basis.Forward.Cross(basis.Up).Normalized();
	const Vector3D up = right.Cross(basis.Forward);
	const float ndcX = 2.0f * (static_cast<float>(x) + 0.5f) / static_cast<float>(faceSize) - 1.0f;
	const float ndcY = 1.0f - 2.0f * (static_cast<float>(y) + 0.5f) / static_cast<float>(faceSize);
	return (basis.Forward + right * ndcX + up * ndcY).Normalized();
}

ProbeCapture::ProbeCapture(U32 faceSize, EProbeFaces faces)
	: m_faceSize(faceSize)
	, m_faces(faces)
{
}

ProbeCapture::~ProbeCapture() = default;

void ProbeCapture::Initialize()
{
	// The forward pass's vertices (ForwardRenderStage), and which face each instance is drawn into from slot 1
	Rendering::ShaderInputLayout inputLayout =
	{
		{ "POSITION", Rendering::EDataFormat::R32G32B32_FLOAT, Rendering::EDataClassification::PER_VERTEX, 0 },
		{ "NORMAL", Rendering::EDataFormat::R32G32B32_FLOAT, Rendering::EDataClassification::PER_VERTEX, 12 },
		{ "UV", Rendering::EDataFormat::R32G32_FLOAT, Rendering::EDataClassification::PER_VERTEX, 24 },
		{ "TANGENT", Rendering::EDataFormat::R32G32B32_FLOAT, Rendering::EDataClassification::PER_VERTEX, 32 },
		{ "FACE", Rendering::EDataFormat::R32_UINT, Rendering::EDataClassification::PER_INSTANCE, 0, 1 }
	};

	m_vertexShader = LoadVertexShader("Vertex_ProbeCapture", inputLayout);
	m_pixelShader = LoadPixelShader("Pixel_ProbeCapture");
	m_faceTableBuffer = Rendering::Renderer::CreateConstantBuffer(nullptr, sizeof(FaceTableLayout), 16, 1);
	m_instanceCapacity = k_initialInstanceCapacity;
	m_instanceBuffer = Rendering::Renderer::CreateDynamicVertexBuffer(m_instanceCapacity * sizeof(U32), sizeof(U32));
}

ProbeCapture::FaceBasis ProbeCapture::GetFaceBasisOf(U32 face) const
{
	if (m_faces == EProbeFaces::Probe)
	{
		return GetFaceBasis(face);
	}

	const ECubeFace cubeFace = static_cast<ECubeFace>(face);
	return { CubeMap::GetFaceForward(cubeFace), CubeMap::GetFaceUp(cubeFace) };
}

U32 ProbeCapture::GetFacesSeeing(const Vector3D& position, const BoundingBox& box) const
{
	if (box.IsEmpty())
	{
		return 0;
	}

	const BoundingBox relativeBox(box.GetMin() - position, box.GetMax() - position);
	U32 faceMask = 0;
	for (U32 face = 0; face < k_faceCount; ++face)
	{
		if (CubeMap::IsInFaceView(GetFaceBasisOf(face).Forward, relativeBox))
		{
			faceMask |= 1u << face;
		}
	}
	return faceMask;
}

void ProbeCapture::BeginBatch(U32 targetWidth, U32 targetHeight)
{
	if (m_batch == nullptr)
	{
		m_batch = std::make_unique<Batch>();
	}

	m_batch->TargetWidth = targetWidth;
	m_batch->TargetHeight = targetHeight;
	m_batch->Probes.clear();
	m_batch->FaceCount = 0;
}

bool ProbeCapture::AddProbe(const Vector3D& position, float farDistance, U32 left, U32 top, U32 faceMask)
{
	AssertNotNull(m_batch);

	U32 faceCount = 0;
	for (U32 face = 0; face < k_faceCount; ++face)
	{
		faceCount += (faceMask >> face) & 1u;
	}
	if (m_batch->FaceCount + faceCount > k_maxBatchFaces)
	{
		return false;
	}

	const Matrix4x4 projection = Matrix4x4::CreatePerspectiveMatrixZeroToOne(90.0f, 1.0f, k_nearDistance, farDistance);
	const float width = static_cast<float>(m_batch->TargetWidth);
	const float height = static_cast<float>(m_batch->TargetHeight);
	const float size = static_cast<float>(m_faceSize);

	Batch::Probe& probe = m_batch->Probes.emplace_back();
	probe.Position = position;
	for (U32 face = 0; face < k_faceCount; ++face)
	{
		if ((faceMask & (1u << face)) == 0)
		{
			probe.FaceIndices[face] = -1;
			continue;
		}

		probe.FaceIndices[face] = static_cast<int>(m_batch->FaceCount);
		FaceLayout& layout = m_batch->FaceTable.Faces[m_batch->FaceCount++];

		const FaceBasis basis = GetFaceBasisOf(face);
		layout.ViewProjection = projection * Matrix4x4::CreateViewMatrix(position, position + basis.Forward, basis.Up);

		// The face's square, from clip space -1 to 1 across and up, to its place in the target's
		const float faceLeft = static_cast<float>(left + face * m_faceSize);
		const float leftX = 2.0f * faceLeft / width - 1.0f;
		const float rightX = 2.0f * (faceLeft + size) / width - 1.0f;
		const float topY = 1.0f - 2.0f * static_cast<float>(top) / height;
		const float bottomY = 1.0f - 2.0f * (static_cast<float>(top) + size) / height;
		layout.Placement[0] = (rightX - leftX) * 0.5f;
		layout.Placement[1] = (topY - bottomY) * 0.5f;
		layout.Placement[2] = (rightX + leftX) * 0.5f;
		layout.Placement[3] = (topY + bottomY) * 0.5f;

		layout.ProbePosition[0] = position.X();
		layout.ProbePosition[1] = position.Y();
		layout.ProbePosition[2] = position.Z();
		layout.ProbePosition[3] = 1.0f;
	}
	return true;
}

U32 ProbeCapture::Flush(const RenderScene& scene)
{
	AssertNotNull(m_batch);
	Batch& batch = *m_batch;
	if (batch.FaceCount == 0)
	{
		return 0;
	}

	// A draw narrower than a texel would cover nothing: its bounds' angular radius, as a tangent, under half a texel's
	const float minAngularRadius = 1.0f / static_cast<float>(m_faceSize);

	// Each draw's instances, a face each that sees it, together
	batch.Instances.clear();
	batch.Draws.clear();
	for (const auto& renderObject : scene.GetRenderObjects())
	{
		// Movable objects are lit by the probes, not captured into them
		if (renderObject->GetMobility() == ERenderMobility::Movable)
		{
			continue;
		}

		for (const MeshDraw& draw : renderObject->GetDraws())
		{
			// See-through surfaces don't block the probes' view, as they don't cast shadows
			if (draw.Slot->GetMaterial().IsBlended())
			{
				continue;
			}

			const U32 firstInstance = static_cast<U32>(batch.Instances.size());
			const Vector3D centre = draw.Bounds.GetCentre();
			const float radius = draw.Bounds.GetRadius();
			for (const Batch::Probe& probe : batch.Probes)
			{
				const float distance = (centre - probe.Position).Length();
				if (distance > radius && radius < distance * minAngularRadius)
				{
					continue;
				}

				const BoundingBox relativeBox(draw.Bounds.GetMin() - probe.Position, draw.Bounds.GetMax() - probe.Position);
				for (U32 face = 0; face < k_faceCount; ++face)
				{
					if (probe.FaceIndices[face] >= 0 && CubeMap::IsInFaceView(GetFaceBasisOf(face).Forward, relativeBox))
					{
						batch.Instances.push_back(static_cast<U32>(probe.FaceIndices[face]));
					}
				}
			}

			const U32 instanceCount = static_cast<U32>(batch.Instances.size()) - firstInstance;
			if (instanceCount > 0)
			{
				batch.Draws.push_back({ &draw, firstInstance, instanceCount });
			}
		}
	}

	if (batch.Draws.empty())
	{
		return 0;
	}

	if (batch.Instances.size() > m_instanceCapacity)
	{
		while (m_instanceCapacity < batch.Instances.size())
		{
			m_instanceCapacity *= 2;
		}
		m_instanceBuffer = Rendering::Renderer::CreateDynamicVertexBuffer(m_instanceCapacity * sizeof(U32), sizeof(U32));
	}
	Rendering::Renderer::UpdateVertexBuffer(m_instanceBuffer, batch.Instances.data(), batch.Instances.size() * sizeof(U32));
	Rendering::Renderer::UploadDataToBuffer<FaceTableLayout>(m_faceTableBuffer, &batch.FaceTable);

	Rendering::Renderer::SetRasterizerState(Rendering::ERasterizerState::NoCull);
	Rendering::Renderer::SetVertexShader(m_vertexShader->GetPlatformObject());
	Rendering::Renderer::SetInputLayout(m_vertexShader->GetPlatformObject());
	Rendering::Renderer::SetPrimitiveTopology(Rendering::EPrimitiveTopology::TriangleList);
	Rendering::Renderer::SetConstantBufferVS(m_vertexShader->GetWorldMatrixBuffer(), 1);
	Rendering::Renderer::SetConstantBufferVS(m_faceTableBuffer, k_faceTableSlot);
	Rendering::Renderer::SetPixelShader(m_pixelShader->GetPlatformObject());
	Rendering::Renderer::SetVertexBuffer(m_instanceBuffer, 1);

	// The whole target: each face places itself in it
	Rendering::ViewportParams viewport;
	viewport.Width = static_cast<float>(batch.TargetWidth);
	viewport.Height = static_cast<float>(batch.TargetHeight);
	Rendering::Renderer::SetViewport(viewport);

	const Matrix4x4* uploadedTransform = nullptr;
	for (const Batch::Draw& batchDraw : batch.Draws)
	{
		const MeshDraw& draw = *batchDraw.Draw;
		if (draw.Transform != uploadedTransform)
		{
			Rendering::Renderer::UploadDataToBuffer<Matrix4x4>(m_vertexShader->GetWorldMatrixBuffer(), draw.Transform);
			uploadedTransform = draw.Transform;
		}

		// Only the albedo and the cutout mask are read
		const MaterialInstance& materialInstance = draw.Slot->GetMaterial();
		Rendering::Renderer::SetConstantBufferPS(materialInstance.GetParamBuffer(), 1);
		if (const Texture2D* const diffuseMap = materialInstance.GetTextureResource(TEXTURE_SLOT_DIFFUSE))
		{
			Rendering::Renderer::SetTextureResource(diffuseMap->GetPlatformObject(), TEXTURE_SLOT_DIFFUSE);
		}
		if (const Texture2D* const opacityMap = materialInstance.GetTextureResource(TEXTURE_SLOT_OPACITY))
		{
			Rendering::Renderer::SetTextureResource(opacityMap->GetPlatformObject(), TEXTURE_SLOT_OPACITY);
		}

		const MeshGeometry& meshGeometry = *draw.Geometry;
		const Rendering::IndexBufferHandle& indexBuffer = meshGeometry.GetIndexBuffer()->GetPlatformObject();
		Rendering::Renderer::SetVertexBuffer(meshGeometry.GetVertexBuffer()->GetPlatformObject(), 0);
		Rendering::Renderer::SetIndexBuffer(indexBuffer);
		Rendering::Renderer::DrawIndexedInstanced(indexBuffer, batchDraw.InstanceCount, batchDraw.FirstInstance);
	}

	return static_cast<U32>(batch.Draws.size());
}
