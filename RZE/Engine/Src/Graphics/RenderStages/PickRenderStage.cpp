#include <StdAfx.h>
#include <Graphics/RenderStages/PickRenderStage.h>

#include <Graphics/IndexBuffer.h>
#include <Graphics/Material.h>
#include <Graphics/MaterialSlot.h>
#include <Graphics/MeshGeometry.h>
#include <Graphics/RenderEngine.h>
#include <Graphics/Shader.h>
#include <Graphics/Texture2D.h>
#include <Graphics/VertexBuffer.h>

#include <Rendering/Renderer.h>
#include <Rendering/Driver/GFXBuffer.h>

#include <Utils/Math/Frustum.h>

void PickRenderStage::Initialize()
{
	// Same layout as ForwardRenderStage; the forward vertex shader is reused with the pick camera
	Rendering::ShaderInputLayout inputLayout =
	{
		{ "POSITION", Rendering::EDataFormat::R32G32B32_FLOAT, Rendering::EDataClassification::PER_VERTEX, 0 },
		{ "NORMAL", Rendering::EDataFormat::R32G32B32_FLOAT, Rendering::EDataClassification::PER_VERTEX, 12 },
		{ "UV", Rendering::EDataFormat::R32G32_FLOAT, Rendering::EDataClassification::PER_VERTEX, 24 },
		{ "TANGENT", Rendering::EDataFormat::R32G32B32_FLOAT, Rendering::EDataClassification::PER_VERTEX, 32 }
	};

	m_vertexShaderResource = RZE().GetResourceHandler().LoadResource<VertexShader>(Filepath("Assets/Shaders/Vertex_NewRenderer.hlsl"), "Vertex_NewRenderer", inputLayout);
	AssertExpr(m_vertexShaderResource.IsValid());
	m_vertexShader = RZE().GetResourceHandler().GetResource<VertexShader>(m_vertexShaderResource);

	m_pixelShaderResource = RZE().GetResourceHandler().LoadResource<PixelShader>(Filepath("Assets/Shaders/Pixel_PickID.hlsl"), "Pixel_PickID");
	AssertExpr(m_pixelShaderResource.IsValid());
	m_pixelShader = RZE().GetResourceHandler().GetResource<PixelShader>(m_pixelShaderResource);

	Rendering::GFXTextureBufferParams idParams = { 0 };
	idParams.Width = 1;
	idParams.Height = 1;
	idParams.MipLevels = 1;
	idParams.SampleCount = 1;
	idParams.Format = Rendering::ETextureFormat::R32_UINT;
	idParams.bIsRenderTarget = true;
	idParams.bIsShaderResource = true;
	m_idTarget = Rendering::Renderer::CreateTextureBuffer2D(nullptr, idParams);

	Rendering::GFXTextureBufferParams depthParams = { 0 };
	depthParams.Width = 1;
	depthParams.Height = 1;
	depthParams.bIsDepthTexture = true;
	// Also gives it the SRV that every texture buffer is expected to have
	depthParams.bIsShaderResource = true;
	m_depthTarget = Rendering::Renderer::CreateTextureBuffer2D(nullptr, depthParams);

	m_cameraBuffer = Rendering::Renderer::CreateConstantBuffer(nullptr, sizeof(RenderCamera), 128, 1);
	m_pickBuffer = Rendering::Renderer::CreateConstantBuffer(nullptr, sizeof(PickBufferLayout), 16, 1);
}

void PickRenderStage::Setup(RenderStageBuilder& builder)
{
	// Works from the scene alone and hands its result straight to the requester
	builder.RunsIn(ERenderViewFilter::MainOnly);
}

void PickRenderStage::Render(RenderContext& context)
{
	OPTICK_EVENT();

	const RenderView& view = context.View;
	if (!view.Pick.has_value())
	{
		return;
	}

	Rendering::Renderer::Begin("PickRenderStage");

	// The requested pixel in NDC; screen y runs down, NDC y runs up
	const RenderPickRequest& pick = *view.Pick;
	const float width = view.ViewportSize.X();
	const float height = view.ViewportSize.Y();
	const float ndcMinX = pick.X / width * 2.0f - 1.0f;
	const float ndcMaxX = (pick.X + 1) / width * 2.0f - 1.0f;
	const float ndcMinY = 1.0f - (pick.Y + 1) / height * 2.0f;
	const float ndcMaxY = 1.0f - pick.Y / height * 2.0f;

	RenderCamera pickCamera = view.Camera;
	pickCamera.ClipSpace = Matrix4x4::CreateSubRectProjection(ndcMinX, ndcMinY, ndcMaxX, ndcMaxY) * view.Camera.ClipSpace;

	Rendering::Renderer::SetColourTarget(m_idTarget, m_depthTarget);
	Rendering::Renderer::ClearRenderTarget(m_idTarget, Vector4D(0.0f, 0.0f, 0.0f, 0.0f));
	Rendering::Renderer::ClearDepthStencilBuffer(m_depthTarget);

	Rendering::Renderer::UploadDataToBuffer<RenderCamera>(m_cameraBuffer, &pickCamera);

	Rendering::Renderer::SetVertexShader(m_vertexShader->GetPlatformObject());
	Rendering::Renderer::SetPixelShader(m_pixelShader->GetPlatformObject());
	Rendering::Renderer::SetConstantBufferVS(m_cameraBuffer, 0);
	Rendering::Renderer::SetConstantBufferVS(m_vertexShader->GetWorldMatrixBuffer(), 1);
	Rendering::Renderer::SetConstantBufferPS(m_pickBuffer, 0);

	Rendering::Renderer::SetViewport({ 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f });
	Rendering::Renderer::SetInputLayout(m_vertexShader->GetPlatformObject());
	Rendering::Renderer::SetPrimitiveTopology(Rendering::EPrimitiveTopology::TriangleList);

	// Only covers the requested pixel, so nearly everything is skipped
	const Frustum frustum(pickCamera.ClipSpace);

	for (const auto& renderObject : context.Scene.GetRenderObjects())
	{
		if (!frustum.Intersects(renderObject->GetBounds()))
		{
			continue;
		}

		// Objects without an ID are still drawn, so they hide what's behind them
		const PickBufferLayout pickData = { renderObject->GetPickID(), { 0, 0, 0 } };
		Rendering::Renderer::UploadDataToBuffer<PickBufferLayout>(m_pickBuffer, &pickData);

		// As in ForwardRenderStage: only uploaded when the draw's transform changes
		const Matrix4x4* uploadedTransform = nullptr;

		for (const MeshDraw& draw : renderObject->GetDraws())
		{
			if (!frustum.Intersects(draw.Bounds))
			{
				continue;
			}

			// See ForwardRenderStage: uploads the whole MatrixMem (transform + inverse)
			if (draw.Transform != uploadedTransform)
			{
				Rendering::Renderer::UploadDataToBuffer<Matrix4x4>(m_vertexShader->GetWorldMatrixBuffer(), draw.Transform);
				uploadedTransform = draw.Transform;
			}

			const MeshGeometry& meshGeometry = *draw.Geometry;
			const MaterialInstance& materialInstance = draw.Slot->GetMaterial();

			// For cutout materials, as in ShadowRenderStage
			Rendering::Renderer::SetConstantBufferPS(materialInstance.GetParamBuffer(), 1);
			if (const Texture2D* const opacityMap = materialInstance.GetTextureResource(MaterialInstance::TEXTURE_SLOT_OPACITY))
			{
				Rendering::Renderer::SetTextureResource(opacityMap->GetPlatformObject(), MaterialInstance::TEXTURE_SLOT_OPACITY);
			}

			const Rendering::IndexBufferHandle& indexBuffer = meshGeometry.GetIndexBuffer()->GetPlatformObject();
			Rendering::Renderer::SetVertexBuffer(meshGeometry.GetVertexBuffer()->GetPlatformObject(), 0);
			Rendering::Renderer::SetIndexBuffer(indexBuffer);

			Rendering::Renderer::DrawIndexed(indexBuffer);
		}
	}

	Rendering::Renderer::ReadTexel(m_idTarget, 0, 0, pick.Result);
	if (pick.DepthResult != nullptr)
	{
		Rendering::Renderer::ReadTexel(m_depthTarget, 0, 0, pick.DepthResult);
	}

	Rendering::Renderer::End();
}

float RenderPickRequest::DecodeDepth(U32 texel)
{
	// PickRenderStage's depth target is D24S8: the depth as a 24-bit unorm in the low bits, the stencil above
	constexpr U32 k_depthMask = 0xFFFFFF;
	return static_cast<float>(texel & k_depthMask) / static_cast<float>(k_depthMask);
}
