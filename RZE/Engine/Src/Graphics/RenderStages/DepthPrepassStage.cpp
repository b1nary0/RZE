#include <Graphics/RenderStages/DepthPrepassStage.h>

#include <Graphics/MeshInstanceFormat.h>
#include <Graphics/RenderEngine.h>
#include <Graphics/Shader.h>

#include <Rendering/Renderer.h>
#include <Rendering/Graphics/RenderTarget.h>

#include <Utils/Math/Frustum.h>

void DepthPrepassStage::Initialize()
{
	// Places vertices as ForwardRenderStage's shader does, to the bit (MeshClipPosition in Common/MeshVertex.hlsli)
	m_vertexShader = LoadVertexShader("Vertex_MeshDepthOnly", MeshInstanceFormats::Depth().InputLayout);

	m_batches.Initialize();
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

	SceneDepthData sceneDepthData;
	sceneDepthData.Depth = view.Target->GetDepthTexturePlatformObject();

	Rendering::Renderer::SetDepthTarget(sceneDepthData.Depth);
	Rendering::Renderer::ClearDepthStencilBuffer(sceneDepthData.Depth);

	// The forward pass uploads the same camera to the same buffer again, so the order doesn't matter
	Rendering::Renderer::UploadDataToBuffer<RenderCamera>(m_vertexShader->GetCameraDataBuffer(), &view.Camera);

	Rendering::Renderer::SetVertexShader(m_vertexShader->GetPlatformObject());
	Rendering::Renderer::SetConstantBufferVS(m_vertexShader->GetCameraDataBuffer(), 0);

	Rendering::Renderer::SetViewport({ view.ViewportSize.X(), view.ViewportSize.Y() });

	Rendering::Renderer::SetInputLayout(m_vertexShader->GetPlatformObject());
	Rendering::Renderer::SetPrimitiveTopology(Rendering::EPrimitiveTopology::TriangleList);

	// What the forward pass sees too, so it draws over exactly this depth. Blended draws don't write depth: whatever
	// they cover still has to be shaded.
	const VisibleDraws visibleDraws = context.GetVisibleDraws({ Frustum(view.Camera.ClipSpace), ERenderMobilityFilter::Any });

	m_batches.Clear();
	for (const MeshDraw* draw : visibleDraws.Opaque)
	{
		m_batches.Add(*draw);
	}
	m_batches.Draw();

	m_sceneDepthOutput.Publish(context, sceneDepthData);

	Rendering::Renderer::End();
}
