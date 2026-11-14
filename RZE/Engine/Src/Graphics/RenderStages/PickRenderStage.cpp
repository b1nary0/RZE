#include <Graphics/RenderStages/PickRenderStage.h>

#include <Graphics/MeshInstanceFormat.h>
#include <Graphics/RenderEngine.h>
#include <Graphics/Shader.h>

#include <Rendering/Renderer.h>
#include <Rendering/Driver/GFXBuffer.h>

#include <Utils/Math/Frustum.h>

void PickRenderStage::Initialize()
{
	// Places vertices as the depth prepass does, with the pick camera
	m_vertexShader = LoadVertexShader("Vertex_MeshID", MeshInstanceFormats::ID().InputLayout);

	m_pixelShader = LoadPixelShader("Pixel_PickID");

	Rendering::GFXTextureBufferParams idParams = { 0 };
	idParams.Width = 1;
	idParams.Height = 1;
	idParams.MipLevels = 1;
	idParams.SampleCount = 1;
	idParams.Format = Rendering::ETextureFormat::RG32_UINT;
	idParams.bIsRenderTarget = true;
	idParams.bIsShaderResource = true;
	m_idTarget = Rendering::Renderer::CreateTextureBuffer2D(idParams);

	Rendering::GFXTextureBufferParams depthParams = { 0 };
	depthParams.Width = 1;
	depthParams.Height = 1;
	depthParams.bIsDepthTexture = true;
	// Also gives it the SRV that every texture buffer is expected to have
	depthParams.bIsShaderResource = true;
	m_depthTarget = Rendering::Renderer::CreateTextureBuffer2D(depthParams);

	m_cameraBuffer = Rendering::Renderer::CreateConstantBuffer(nullptr, sizeof(RenderCamera), 128, 1);

	m_batches.Initialize();
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
	if (!view.Frame.Pick.has_value())
	{
		return;
	}

	Rendering::Renderer::Begin("PickRenderStage");

	// The requested pixel in NDC; screen y runs down, NDC y runs up
	const RenderPickRequest& pick = *view.Frame.Pick;
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

	Rendering::Renderer::SetViewport({ 1.0f, 1.0f });
	Rendering::Renderer::SetInputLayout(m_vertexShader->GetPlatformObject());
	Rendering::Renderer::SetPrimitiveTopology(Rendering::EPrimitiveTopology::TriangleList);

	// Only covers the requested pixel, so nearly everything is skipped. Its own cull rather than the view's
	// (VisibleDrawsCache): a draw doesn't know its object, and the pick ID is the object's.
	const Frustum frustum(pickCamera.ClipSpace);

	m_batches.Clear();
	for (const auto& renderObject : context.Scene.GetRenderObjects())
	{
		if (!frustum.Intersects(renderObject->GetBounds()))
		{
			continue;
		}

		// Objects without an ID are still drawn, so they hide what's behind them
		const U64 pickID = renderObject->GetPickID();
		for (const MeshDraw& draw : renderObject->GetDraws())
		{
			if (frustum.Intersects(draw.Bounds))
			{
				m_batches.Add(draw, pickID);
			}
		}
	}
	m_batches.Draw();

	Rendering::Renderer::ReadTexel(m_idTarget, 0, 0, pick.Result);
	if (pick.DepthResult != nullptr)
	{
		Rendering::Renderer::ReadTexel(m_depthTarget, 0, 0, pick.DepthResult);
	}

	Rendering::Renderer::End();
}

float RenderPickRequest::DecodeDepth(U64 texel)
{
	// PickRenderStage's depth target is D24S8: the depth as a 24-bit unorm in the low bits, the stencil above
	constexpr U64 k_depthMask = 0xFFFFFF;
	return static_cast<float>(texel & k_depthMask) / static_cast<float>(k_depthMask);
}
