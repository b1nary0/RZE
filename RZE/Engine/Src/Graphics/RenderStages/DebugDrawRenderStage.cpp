#include <StdAfx.h>
#include <Graphics/RenderStages/DebugDrawRenderStage.h>

#include <Rendering/Renderer.h>

#include <Graphics/Shader.h>

void DebugDrawRenderStage::Initialize()
{
	Rendering::ShaderInputLayout inputLayout =
	{
		{ "POSITION", Rendering::EDataFormat::R32G32B32_FLOAT, Rendering::EDataClassification::PER_VERTEX, 0 },
		{ "COLOUR", Rendering::EDataFormat::R32G32B32_FLOAT, Rendering::EDataClassification::PER_VERTEX, 12 },
	};

	m_vertexShaderResource = RZE().GetResourceHandler().LoadResource<VertexShader>(Filepath("Assets/Shaders/Vertex_Line.hlsl"), "Vertex_Line", inputLayout);
	AssertExpr(m_vertexShaderResource.IsValid());
	m_vertexShader = RZE().GetResourceHandler().GetResource<VertexShader>(m_vertexShaderResource);

	m_lineShaderResource = RZE().GetResourceHandler().LoadResource<PixelShader>(Filepath("Assets/Shaders/Pixel_Line.hlsl"), "Pixel_Line");
	AssertExpr(m_lineShaderResource.IsValid());
	m_lineShader = RZE().GetResourceHandler().GetResource<PixelShader>(m_lineShaderResource);

	// 128 lines minimum; shrink window of 300 Render() calls is ~5s at 60Hz with a single view
	m_lineBuffer.Initialize(sizeof(LineVertex), BufferCapacitySettings{ 256, 300, 4 });
}

void DebugDrawRenderStage::Setup(RenderStageBuilder& builder)
{
	// Drawn after tonemapping, so lines keep their exact colours
	m_displayColour = builder.Modifies<DisplayColourData>();
}

void DebugDrawRenderStage::Render(RenderContext& context)
{
	OPTICK_EVENT();

	const RenderView& view = context.View;
	const RenderEngine::SceneData& renderData = context.Scene;

	m_scratchVertices.clear();
	for (const DebugLine& line : renderData.debugLines)
	{
		m_scratchVertices.push_back({ { line.start.X(), line.start.Y(), line.start.Z() }, { line.colour.X(), line.colour.Y(), line.colour.Z() } });
		m_scratchVertices.push_back({ { line.end.X(), line.end.Y(), line.end.Z() }, { line.colour.X(), line.colour.Y(), line.colour.Z() } });
	}

	// Uploaded even when empty so line-free calls still count toward shrinking
	m_lineBuffer.Upload(m_scratchVertices);

	if (m_lineBuffer.GetCount() == 0)
	{
		return;
	}

	const DisplayColourData& displayColour = m_displayColour.Get(context);

	Rendering::Renderer::Begin("DebugDrawRenderStage");

	// Into the 8-bit target, depth-tested against the scene
	Rendering::Renderer::SetColourTarget(displayColour.Colour, displayColour.Depth);

	Rendering::Renderer::SetPrimitiveTopology(Rendering::EPrimitiveTopology::LineList);
	Rendering::Renderer::UploadDataToBuffer<RenderCamera>(m_vertexShader->GetCameraDataBuffer(), &view.Camera);

	Rendering::Renderer::SetVertexShader(m_vertexShader->GetPlatformObject());
	Rendering::Renderer::SetConstantBufferVS(m_vertexShader->GetCameraDataBuffer(), 0);

	Rendering::Renderer::SetViewport({ view.ViewportSize.X(), view.ViewportSize.Y(), 0.0f, 1.0f, 0.0f, 0.0f });

	Rendering::Renderer::SetInputLayout(m_vertexShader->GetPlatformObject());

	Matrix4x4 transform = Matrix4x4::CreateInPlace(Vector3D(), Vector3D(1.0f), Vector3D());

	Rendering::Renderer::UploadDataToBuffer<Matrix4x4>(m_vertexShader->GetWorldMatrixBuffer(), &transform);
	Rendering::Renderer::SetConstantBufferVS(m_vertexShader->GetWorldMatrixBuffer(), 1);

	Rendering::Renderer::SetPixelShader(m_lineShader->GetPlatformObject());

	Rendering::Renderer::SetVertexBuffer(m_lineBuffer.GetPlatformObject(), 0);
	Rendering::Renderer::Draw(m_lineBuffer.GetPlatformObject(), m_lineBuffer.GetCount());

	Rendering::Renderer::End();
}
