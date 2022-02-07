#include <StdAfx.h>
#include <Graphics/RenderEngine.h>

#include <Graphics/IndexBuffer.h>
#include <Graphics/Material.h>
#include <Graphics/Shader.h>
#include <Graphics/Texture2D.h>

#include <Utils/Math/Vector4D.h>

#include <Rendering/Renderer.h>

RenderEngine::RenderEngine()
{	
}

RenderEngine::~RenderEngine()
{
}

void RenderEngine::Initialize(void* windowHandle)
{
	Rendering::Renderer::Initialize(windowHandle);

	m_vertexShaderResource = RZE::GetResourceHandler().LoadResource<VertexShader>(FilePath("Assets/Shaders/Vertex_NewRenderer.hlsl"), "Vertex_NewRenderer");
	AssertExpr(m_vertexShaderResource.IsValid());
	m_vertexShader = RZE::GetResourceHandler().GetResource<VertexShader>(m_vertexShaderResource);
}

void RenderEngine::Update()
{
	OPTICK_EVENT();

	Rendering::Renderer::UploadDataToBuffer(m_vertexShader->GetCameraDataBuffer(), &m_camera);
	
#ifdef IMGUI_ENABLED
	ImGui::ShowDemoWindow();
#endif
}

void RenderEngine::Render()
{
	OPTICK_EVENT();

	Rendering::Renderer::BeginFrame();

	Rendering::Renderer::SetVertexShader(m_vertexShader->GetPlatformObject());
	Rendering::Renderer::SetConstantBufferVS(m_vertexShader->GetCameraDataBuffer(), 0);

	Rendering::Renderer::Begin();

	Rendering::Renderer::SetClearColour(Vector4D(0.5f, 0.5f, 1.0f, 1.0f));
	Rendering::Renderer::SetViewport({ m_canvasSize.X(), m_canvasSize.Y(), 0.0f, 1.0f, 0.0f, 0.0f });

	Rendering::Renderer::SetInputLayout(m_vertexShader->GetPlatformObject());
	Rendering::Renderer::SetPrimitiveTopology(Rendering::EPrimitiveTopology::TriangleList);

	for (auto& renderObject : m_renderObjects)
	{
		struct MatrixMem
		{
			Matrix4x4 world;
			Matrix4x4 inverseWorld;
		} matrixMem;

		matrixMem.world = renderObject->GetTransform();
		matrixMem.inverseWorld = renderObject->GetTransform().Inverse();

		Rendering::Renderer::UploadDataToBuffer(m_vertexShader->GetWorldMatrixBuffer(), &matrixMem);

		// @TODO
		// Currently each MeshGeometry is a draw call. Need to batch this down so it becomes a single draw call
		// per render object, at least.
		for (auto& meshGeometry : renderObject->GetStaticMesh().GetSubMeshes())
		{
			// @TODO
			// This is god awful. Just in place while developing shader model.
			// Should get resolved once the system matures
			std::shared_ptr<Material> material = meshGeometry.GetMaterial();
			const Material::MaterialProperties& materialProperties = material->GetProperties();
			const PixelShader* const pixelShader = RZE::GetResourceHandler().GetResource<PixelShader>(material->GetShaderResource());

			Rendering::Renderer::SetPixelShader(pixelShader->GetPlatformObject());

			// @TODO This upload call should be moved when MaterialInstance is a thing.
			Rendering::Renderer::UploadDataToBuffer(pixelShader->GetMaterialBuffer(), &materialProperties);
			
			Rendering::Renderer::SetConstantBufferVS(m_vertexShader->GetWorldMatrixBuffer(), 1);
			Rendering::Renderer::SetConstantBufferPS(m_vertexShader->GetCameraDataBuffer(), 0);
			Rendering::Renderer::SetConstantBufferPS(pixelShader->GetMaterialBuffer(), 1);

			// @TODO Really need to get to texture infrastructure refactor soon - 2/6/2022
			for (U8 textureSlot = 0; textureSlot < Material::TextureSlot::TEXTURE_SLOT_COUNT; ++textureSlot)
			{
				const Texture2D* const texture = RZE::GetResourceHandler().GetResource<Texture2D>(material->GetTexture(textureSlot));
				if (texture != nullptr)
				{
					// @TODO Should solve this better by providing an API that will provide a texture resource array
					Rendering::Renderer::SetTextureResource(texture->GetPlatformObject(), textureSlot);
				}
			}

			Rendering::Renderer::SetVertexBuffer(meshGeometry.GetVertexBuffer()->GetPlatformObject(), 0);
			Rendering::Renderer::SetIndexBuffer(meshGeometry.GetIndexBuffer()->GetPlatformObject());

			Rendering::Renderer::DrawIndexed(meshGeometry.GetIndexBuffer()->GetPlatformObject());
		}
	}

	Rendering::Renderer::End();

#ifdef IMGUI_ENABLED
	{
		OPTICK_EVENT("ImGui Render");
		ImGui::Render();
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	}
#endif

	Rendering::Renderer::EndFrame();
}

void RenderEngine::Shutdown()
{
	Rendering::Renderer::Shutdown();
}

std::shared_ptr<RenderObject> RenderEngine::CreateRenderObject(const StaticMesh& staticMesh)
{
	OPTICK_EVENT();

	m_renderObjects.emplace_back(std::make_shared<RenderObject>());
	m_renderObjects.back()->SetStaticMesh(staticMesh);

	return m_renderObjects.back();
}

void RenderEngine::ResizeCanvas(const Vector2D& newSize)
{
	m_canvasSize = newSize;

#ifdef IMGUI_ENABLED
	ImGui::GetIO().DisplaySize.x = m_canvasSize.X();
	ImGui::GetIO().DisplaySize.y = m_canvasSize.Y();
#endif

	Rendering::Renderer::HandleWindowResize(m_canvasSize);

	LOG_CONSOLE_ARGS("New Canvas Size: %f x %f", m_canvasSize.X(), m_canvasSize.Y());
}
