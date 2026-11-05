#include <Graphics/RenderStages/DepthPrepassStage.h>

#include <Graphics/IndexBuffer.h>
#include <Graphics/Material.h>
#include <Graphics/MaterialDefines.h>
#include <Graphics/MaterialSlot.h>
#include <Graphics/RenderEngine.h>
#include <Graphics/Shader.h>
#include <Graphics/Texture2D.h>
#include <Graphics/VertexBuffer.h>

#include <Rendering/Renderer.h>
#include <Rendering/Graphics/RenderTarget.h>

#include <Utils/Math/Frustum.h>

void DepthPrepassStage::Initialize()
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
	m_cutoutShader = LoadPixelShader("Pixel_DepthPrepass");
}

void DepthPrepassStage::Setup(RenderStageBuilder& builder)
{
	m_sceneDepthOutput = builder.Writes<SceneDepthData>();
}

void DepthPrepassStage::Render(RenderContext& context)
{
	OPTICK_EVENT();

	Rendering::Renderer::Begin("DepthPrepassStage");

	const RenderView& view = context.View;
	const RenderScene& scene = context.Scene;

	SceneDepthData sceneDepthData;
	sceneDepthData.Depth = view.Target->GetDepthTexturePlatformObject();

	Rendering::Renderer::SetDepthTarget(sceneDepthData.Depth);
	Rendering::Renderer::ClearDepthStencilBuffer(sceneDepthData.Depth);

	// The forward pass uploads the same camera to the same buffer again, so the order doesn't matter
	Rendering::Renderer::UploadDataToBuffer<RenderCamera>(m_vertexShader->GetCameraDataBuffer(), &view.Camera);

	Rendering::Renderer::SetVertexShader(m_vertexShader->GetPlatformObject());
	Rendering::Renderer::SetConstantBufferVS(m_vertexShader->GetCameraDataBuffer(), 0);
	Rendering::Renderer::SetConstantBufferVS(m_vertexShader->GetWorldMatrixBuffer(), 1);

	Rendering::Renderer::SetViewport({ view.ViewportSize.X(), view.ViewportSize.Y() });

	Rendering::Renderer::SetInputLayout(m_vertexShader->GetPlatformObject());
	Rendering::Renderer::SetPrimitiveTopology(Rendering::EPrimitiveTopology::TriangleList);

	// Opaque draws need no pixel shader, so it's only set when a cutout's turn comes, and unset after
	bool isCutoutShaderBound = false;
	Rendering::Renderer::UnsetPixelShader();

	// As in ForwardRenderStage: only uploaded when the draw's transform changes
	const Matrix4x4* uploadedTransform = nullptr;

	// The same culling as the forward pass, so it draws over exactly this depth
	const Frustum frustum(view.Camera.ClipSpace);

	for (const auto& renderObject : scene.GetRenderObjects())
	{
		if (!frustum.Intersects(renderObject->GetBounds()))
		{
			continue;
		}

		for (const MeshDraw& draw : renderObject->GetDraws())
		{
			const MaterialInstance& materialInstance = draw.Slot->GetMaterial();

			// Blended draws don't write depth: whatever they cover still has to be shaded
			if (materialInstance.IsBlended() || !frustum.Intersects(draw.Bounds))
			{
				continue;
			}

			if (draw.Transform != uploadedTransform)
			{
				Rendering::Renderer::UploadDataToBuffer<Matrix4x4>(m_vertexShader->GetWorldMatrixBuffer(), draw.Transform);
				uploadedTransform = draw.Transform;
			}

			const Texture2D* const opacityMap = materialInstance.GetTextureResource(TEXTURE_SLOT_OPACITY);
			const bool isCutout = opacityMap != nullptr;
			if (isCutout != isCutoutShaderBound)
			{
				if (isCutout)
				{
					Rendering::Renderer::SetPixelShader(m_cutoutShader->GetPlatformObject());
				}
				else
				{
					Rendering::Renderer::UnsetPixelShader();
				}
				isCutoutShaderBound = isCutout;
			}

			if (isCutout)
			{
				Rendering::Renderer::SetConstantBufferPS(materialInstance.GetParamBuffer(), 1);
				Rendering::Renderer::SetTextureResource(opacityMap->GetPlatformObject(), TEXTURE_SLOT_OPACITY);
			}

			const MeshGeometry& meshGeometry = *draw.Geometry;
			const Rendering::IndexBufferHandle& indexBuffer = meshGeometry.GetIndexBuffer()->GetPlatformObject();
			Rendering::Renderer::SetVertexBuffer(meshGeometry.GetVertexBuffer()->GetPlatformObject(), 0);
			Rendering::Renderer::SetIndexBuffer(indexBuffer);

			Rendering::Renderer::DrawIndexed(indexBuffer);
		}
	}

	m_sceneDepthOutput.Publish(context, sceneDepthData);

	Rendering::Renderer::End();
}
