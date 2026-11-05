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

#include <Utils/Math/Frustum.h>
#include <Utils/Math/Matrix4x4.h>

namespace
{
	// Must match ProbeCaptureBuffer's register in Pixel_ProbeCapture.hlsl
	constexpr U32 k_paramsSlot = 5;
	// Close enough for a probe a few centimetres from a wall to still see it
	constexpr float k_nearDistance = 0.02f;
}

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

void ProbeCapture::Initialize()
{
	// Must match ForwardRenderStage's, which loads the same shader
	Rendering::ShaderInputLayout inputLayout =
	{
		{ "POSITION", Rendering::EDataFormat::R32G32B32_FLOAT, Rendering::EDataClassification::PER_VERTEX, 0 },
		{ "NORMAL", Rendering::EDataFormat::R32G32B32_FLOAT, Rendering::EDataClassification::PER_VERTEX, 12 },
		{ "UV", Rendering::EDataFormat::R32G32_FLOAT, Rendering::EDataClassification::PER_VERTEX, 24 },
		{ "TANGENT", Rendering::EDataFormat::R32G32B32_FLOAT, Rendering::EDataClassification::PER_VERTEX, 32 }
	};

	m_vertexShader = LoadVertexShader("Vertex_NewRenderer", inputLayout);
	m_pixelShader = LoadPixelShader("Pixel_ProbeCapture");
	m_cameraBuffer = Rendering::Renderer::CreateConstantBuffer(nullptr, sizeof(RenderCamera), 128, 1);
	m_paramsBuffer = Rendering::Renderer::CreateConstantBuffer(nullptr, sizeof(ParamsLayout), 16, 1);
}

U32 ProbeCapture::Capture(const RenderScene& scene, const Vector3D& position, float farDistance, U32 left, U32 top)
{
	Rendering::Renderer::SetRasterizerState(Rendering::ERasterizerState::NoCull);
	Rendering::Renderer::SetVertexShader(m_vertexShader->GetPlatformObject());
	Rendering::Renderer::SetInputLayout(m_vertexShader->GetPlatformObject());
	Rendering::Renderer::SetPrimitiveTopology(Rendering::EPrimitiveTopology::TriangleList);
	Rendering::Renderer::SetConstantBufferVS(m_cameraBuffer, 0);
	Rendering::Renderer::SetConstantBufferVS(m_vertexShader->GetWorldMatrixBuffer(), 1);
	Rendering::Renderer::SetPixelShader(m_pixelShader->GetPlatformObject());

	const ParamsLayout params = { { position.X(), position.Y(), position.Z() }, 0.0f };
	Rendering::Renderer::UploadDataToBuffer<ParamsLayout>(m_paramsBuffer, &params);
	Rendering::Renderer::SetConstantBufferPS(m_paramsBuffer, k_paramsSlot);

	// A draw narrower than a texel would cover nothing: its bounds' angular radius, as a tangent, under half a texel's
	const float minAngularRadius = 1.0f / static_cast<float>(m_faceSize);
	const float size = static_cast<float>(m_faceSize);
	const Matrix4x4 projection = Matrix4x4::CreatePerspectiveMatrixZeroToOne(90.0f, 1.0f, k_nearDistance, farDistance);

	U32 drawCount = 0;
	for (U32 face = 0; face < k_faceCount; ++face)
	{
		const ECubeFace cubeFace = static_cast<ECubeFace>(face);
		const FaceBasis basis = m_faces == EProbeFaces::Probe ? GetFaceBasis(face) : FaceBasis{ CubeMap::GetFaceForward(cubeFace), CubeMap::GetFaceUp(cubeFace) };

		RenderCamera camera;
		camera.Projection = projection;
		camera.ClipSpace = projection * Matrix4x4::CreateViewMatrix(position, position + basis.Forward, basis.Up);
		camera.Position = position;
		Rendering::Renderer::UploadDataToBuffer<RenderCamera>(m_cameraBuffer, &camera);

		Rendering::ViewportParams viewport;
		viewport.Width = size;
		viewport.Height = size;
		viewport.TopLeftX = static_cast<float>(left + face * m_faceSize);
		viewport.TopLeftY = static_cast<float>(top);
		Rendering::Renderer::SetViewport(viewport);

		const Frustum frustum(camera.ClipSpace);
		const Matrix4x4* uploadedTransform = nullptr;

		for (const auto& renderObject : scene.GetRenderObjects())
		{
			if (!frustum.Intersects(renderObject->GetBounds()))
			{
				continue;
			}

			for (const MeshDraw& draw : renderObject->GetDraws())
			{
				const MaterialInstance& materialInstance = draw.Slot->GetMaterial();
				// See-through surfaces don't block the probe's view, as they don't cast shadows
				if (materialInstance.IsBlended() || !frustum.Intersects(draw.Bounds))
				{
					continue;
				}

				const float distance = (draw.Bounds.GetCentre() - position).Length();
				const float radius = draw.Bounds.GetRadius();
				if (distance > radius && radius < distance * minAngularRadius)
				{
					continue;
				}

				if (draw.Transform != uploadedTransform)
				{
					Rendering::Renderer::UploadDataToBuffer<Matrix4x4>(m_vertexShader->GetWorldMatrixBuffer(), draw.Transform);
					uploadedTransform = draw.Transform;
				}

				// Only the albedo and the cutout mask are read
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
				Rendering::Renderer::DrawIndexed(indexBuffer);
				++drawCount;
			}
		}
	}

	return drawCount;
}
