#include <StdAfx.h>
#include <Graphics/RenderStages/ForwardRenderStage.h>

#include <Graphics/IndexBuffer.h>
#include <Graphics/Material.h>
#include <Graphics/Shader.h>
#include <Graphics/Texture2D.h>
#include <Graphics/VertexBuffer.h>

#include <Rendering/Renderer.h>

#include "Graphics/RenderEngine.h"

void ForwardRenderStage::Initialize()
{
	Rendering::ShaderInputLayout inputLayout =
	{
		{ "POSITION", Rendering::EDataFormat::R32G32B32_FLOAT, Rendering::EDataClassification::PER_VERTEX, 0 },
		{ "NORMAL", Rendering::EDataFormat::R32G32B32_FLOAT, Rendering::EDataClassification::PER_VERTEX, 12 },
		{ "UV", Rendering::EDataFormat::R32G32B32_FLOAT, Rendering::EDataClassification::PER_VERTEX, 24 },
		{ "TANGENT", Rendering::EDataFormat::R32G32B32_FLOAT, Rendering::EDataClassification::PER_VERTEX, 32 }
	};

	m_vertexShaderResource = RZE::GetResourceHandler().LoadResource<VertexShader>(FilePath("Assets/Shaders/Vertex_NewRenderer.hlsl"), "Vertex_NewRenderer", inputLayout);
	AssertExpr(m_vertexShaderResource.IsValid());
	m_vertexShader = RZE::GetResourceHandler().GetResource<VertexShader>(m_vertexShaderResource);
}

void ForwardRenderStage::Update(const std::vector<std::shared_ptr<RenderObject>>& renderObjects)
{
	const RenderCamera& renderCamera = RZE::GetRenderEngine().GetCamera();
	Rendering::Renderer::UploadDataToBuffer(m_vertexShader->GetCameraDataBuffer(), &renderCamera);
}

void ForwardRenderStage::Render(const std::vector<std::shared_ptr<RenderObject>>& renderObjects)
{
	const RenderEngine& renderEngine = RZE::GetRenderEngine();
	Rendering::Renderer::SetVertexShader(m_vertexShader->GetPlatformObject());
	Rendering::Renderer::SetConstantBufferVS(m_vertexShader->GetCameraDataBuffer(), 0);

	Rendering::Renderer::Begin();

	Rendering::Renderer::SetClearColour(Vector4D(0.5f, 0.5f, 1.0f, 1.0f));
	Rendering::Renderer::SetViewport({ renderEngine.GetCanvasSize().X(), renderEngine.GetCanvasSize().Y(), 0.0f, 1.0f, 0.0f, 0.0f});

	Rendering::Renderer::SetInputLayout(m_vertexShader->GetPlatformObject());
	Rendering::Renderer::SetPrimitiveTopology(Rendering::EPrimitiveTopology::TriangleList);

	for (auto& renderObject : renderObjects)
	{
		struct MatrixMem
		{
			Matrix4x4 world;
			Matrix4x4 inverseWorld;
		} matrixMem;

		matrixMem.world = renderObject->GetTransform();
		matrixMem.inverseWorld = renderObject->GetTransform().Inverse();

		Rendering::Renderer::UploadDataToBuffer(m_vertexShader->GetWorldMatrixBuffer(), &matrixMem);
		Rendering::Renderer::SetConstantBufferVS(m_vertexShader->GetWorldMatrixBuffer(), 1);

		// @TODO
		// Currently each MeshGeometry is a draw call. Need to batch this down so it becomes a single draw call
		// per render object, at least.
		for (auto& meshGeometry : renderObject->GetStaticMesh().GetSubMeshes())
		{
			// @TODO
			// This is god awful. Just in place while developing shader model.
			// Should get resolved once the system matures
			std::shared_ptr<MaterialInstance> materialInstance = meshGeometry.GetMaterial();
			const PixelShader* const pixelShader = RZE::GetResourceHandler().GetResource<PixelShader>(materialInstance->GetShaderResource());

			Rendering::Renderer::SetPixelShader(pixelShader->GetPlatformObject());
			Rendering::Renderer::SetConstantBufferPS(materialInstance->GetParamBuffer(), 1);

			// @TODO Really need to get to texture infrastructure refactor soon - 2/6/2022
			for (U8 textureSlot = 0; textureSlot < MaterialInstance::TextureSlot::TEXTURE_SLOT_COUNT; ++textureSlot)
			{
				ResourceHandle resourceHandle = materialInstance->GetTexture(textureSlot);
				if (resourceHandle.IsValid())
				{
					const Texture2D* const texture = RZE::GetResourceHandler().GetResource<Texture2D>(resourceHandle);

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
}