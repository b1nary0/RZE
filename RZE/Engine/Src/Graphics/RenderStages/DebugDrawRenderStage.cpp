#include <StdAfx.h>
#include <Graphics/RenderStages/DebugDrawRenderStage.h>

#include <Rendering/Renderer.h>

#include <Graphics/Shader.h>
#include <Graphics/VertexBuffer.h>
#include <Graphics/IndexBuffer.h>

MeshGeometry s_testLineGeo;

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
}

void DebugDrawRenderStage::Render(const RenderCamera& camera, const RenderEngine::SceneData& renderData)
{
	OPTICK_EVENT();

	static bool init = false;
	if (!init)
	{
		MeshVertex start;
		MeshVertex end;
		start.Position = Vector3D(0.0f, 0.0f, -2.45f);//camera.Position;
		//end.Position = Vector3D(-22.730f, 10.736f, -9.061f);//Vector3D(-7.036f, 3.882f, -0.716f);
		end.Position = Vector3D(-42.541f, 17.410f, 4.407f);

		s_testLineGeo.AddVertex(start);
		s_testLineGeo.AddVertex(end);
		s_testLineGeo.AddIndex(0);
		s_testLineGeo.AddIndex(1);
		s_testLineGeo.AllocateData();

		init = true;
	}

	RenderEngine& renderEngine = RZE().GetRenderEngine();

	Rendering::Renderer::Begin("DebugDrawRenderStage");

	Rendering::Renderer::SetPrimitiveTopology(Rendering::EPrimitiveTopology::LineList);
	Rendering::Renderer::UploadDataToBuffer<RenderCamera>(m_vertexShader->GetCameraDataBuffer(), &camera);

	Rendering::Renderer::SetVertexShader(m_vertexShader->GetPlatformObject());
	Rendering::Renderer::SetConstantBufferVS(m_vertexShader->GetCameraDataBuffer(), 0);

	Rendering::Renderer::SetViewport({ renderEngine.GetViewportSize().X(), renderEngine.GetViewportSize().Y(), 0.0f, 1.0f, 0.0f, 0.0f });

	Rendering::Renderer::SetInputLayout(m_vertexShader->GetPlatformObject());

	Matrix4x4 transform = Matrix4x4::CreateInPlace(Vector3D(), Vector3D(1.0f), Vector3D());
	//Matrix4x4 transform = Matrix4x4::CreateOrthoMatrix(0, renderEngine.GetViewportSize().X() / 2.0f, renderEngine.GetViewportSize().Y() / 2.0f, 0, 0, 0.25);
	//Matrix4x4 transform = Matrix4x4::CreateOrthoMatrix(0, 0.25f, 0.25f, 0, 0, 1);

	Rendering::Renderer::UploadDataToBuffer<Matrix4x4>(m_vertexShader->GetWorldMatrixBuffer(), &transform);
	Rendering::Renderer::SetConstantBufferVS(m_vertexShader->GetWorldMatrixBuffer(), 1);

	Rendering::Renderer::SetPixelShader(m_lineShader->GetPlatformObject());

	Rendering::Renderer::SetVertexBuffer(s_testLineGeo.GetVertexBuffer()->GetPlatformObject(), 0);
	Rendering::Renderer::DrawIndexed(s_testLineGeo.GetIndexBuffer()->GetPlatformObject());

	Rendering::Renderer::End();
}
