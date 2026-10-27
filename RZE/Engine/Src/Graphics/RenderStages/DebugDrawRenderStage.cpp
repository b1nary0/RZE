#include <StdAfx.h>
#include <Graphics/RenderStages/DebugDrawRenderStage.h>

#include <Rendering/Renderer.h>

#include <Graphics/Shader.h>

void DebugDrawRenderStage::Initialize()
{
	Rendering::ShaderInputLayout inputLayout =
	{
		{ "POSITION", Rendering::EDataFormat::R32G32B32_FLOAT, Rendering::EDataClassification::PER_VERTEX, 0 },
		{ "COLOUR", Rendering::EDataFormat::R32G32B32A32_FLOAT, Rendering::EDataClassification::PER_VERTEX, 12 },
		{ "FADECENTRE", Rendering::EDataFormat::R32G32B32_FLOAT, Rendering::EDataClassification::PER_VERTEX, 28 },
		{ "FADERANGE", Rendering::EDataFormat::R32G32_FLOAT, Rendering::EDataClassification::PER_VERTEX, 40 },
	};

	m_vertexShader = LoadVertexShader("Vertex_Line", inputLayout);

	m_lineShader = LoadPixelShader("Pixel_Line");

	// 128 lines minimum; shrink window of 300 Render() calls is ~5s at 60Hz with a single view
	m_lineBuffer.Initialize(sizeof(LineVertex), BufferCapacitySettings{ 256, 300, 4 });
	m_occludedLineBuffer.Initialize(sizeof(LineVertex), BufferCapacitySettings{ 256, 300, 4 });
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
	if (!view.DrawsDebug)
	{
		return;
	}

	auto addLine = [](std::vector<LineVertex>& vertices, const DebugLine& line, float alpha)
	{
		const DebugLineFade& fade = line.fade;
		vertices.push_back({ { line.start.X(), line.start.Y(), line.start.Z() }, { line.colour.X(), line.colour.Y(), line.colour.Z(), alpha },
			{ fade.centre.X(), fade.centre.Y(), fade.centre.Z() }, { fade.start, fade.end } });
		vertices.push_back({ { line.end.X(), line.end.Y(), line.end.Z() }, { line.colour.X(), line.colour.Y(), line.colour.Z(), alpha },
			{ fade.centre.X(), fade.centre.Y(), fade.centre.Z() }, { fade.start, fade.end } });
	};

	m_scratchVertices.clear();
	m_scratchOccludedVertices.clear();
	for (const DebugLine& line : context.Scene.GetDebugLines())
	{
		addLine(m_scratchVertices, line, line.alpha);
		if (line.occludedAlpha > 0.0f)
		{
			addLine(m_scratchOccludedVertices, line, line.occludedAlpha);
		}
	}

	// Uploaded even when empty so line-free calls still count toward shrinking
	m_lineBuffer.Upload(m_scratchVertices);
	m_occludedLineBuffer.Upload(m_scratchOccludedVertices);

	if (m_lineBuffer.GetCount() == 0)
	{
		return;
	}

	const DisplayColourData& displayColour = m_displayColour.Get(context);

	Rendering::Renderer::Begin("DebugDrawRenderStage");

	// Into the 8-bit target, depth-tested against the scene
	Rendering::Renderer::SetColourTarget(displayColour.Colour, displayColour.Depth);
	// Opaque lines (alpha 1) come out the same as without blending
	Rendering::Renderer::SetBlendState(Rendering::EBlendState::AlphaBlend);

	Rendering::Renderer::SetPrimitiveTopology(Rendering::EPrimitiveTopology::LineList);
	Rendering::Renderer::UploadDataToBuffer<RenderCamera>(m_vertexShader->GetCameraDataBuffer(), &view.Camera);

	Rendering::Renderer::SetVertexShader(m_vertexShader->GetPlatformObject());
	Rendering::Renderer::SetConstantBufferVS(m_vertexShader->GetCameraDataBuffer(), 0);

	Rendering::Renderer::SetViewport({ view.ViewportSize.X(), view.ViewportSize.Y() });

	Rendering::Renderer::SetInputLayout(m_vertexShader->GetPlatformObject());

	Matrix4x4 transform = Matrix4x4::CreateInPlace(Vector3D(), Vector3D(1.0f), Vector3D());

	Rendering::Renderer::UploadDataToBuffer<Matrix4x4>(m_vertexShader->GetWorldMatrixBuffer(), &transform);
	Rendering::Renderer::SetConstantBufferVS(m_vertexShader->GetWorldMatrixBuffer(), 1);

	Rendering::Renderer::SetPixelShader(m_lineShader->GetPlatformObject());

	Rendering::Renderer::SetVertexBuffer(m_lineBuffer.GetPlatformObject(), 0);
	Rendering::Renderer::Draw(m_lineBuffer.GetPlatformObject(), m_lineBuffer.GetCount());

	// Again without the depth test, fainter, for lines that show through the scene. It also goes over their visible
	// parts, which are already drawn more opaque, so it barely changes those.
	if (m_occludedLineBuffer.GetCount() > 0)
	{
		Rendering::Renderer::SetDepthState(Rendering::EDepthState::Off);
		Rendering::Renderer::SetVertexBuffer(m_occludedLineBuffer.GetPlatformObject(), 0);
		Rendering::Renderer::Draw(m_occludedLineBuffer.GetPlatformObject(), m_occludedLineBuffer.GetCount());
		Rendering::Renderer::SetDepthState(Rendering::EDepthState::ReadWrite);
	}

	Rendering::Renderer::SetBlendState(Rendering::EBlendState::Opaque);

	Rendering::Renderer::End();
}
