#include <StdAfx.h>
#include <Graphics/RenderStages/ForwardRenderStage.h>

#include <Graphics/IndexBuffer.h>
#include <Graphics/Material.h>
#include <Graphics/RenderEngine.h>
#include <Graphics/Shader.h>
#include <Graphics/Texture2D.h>
#include <Graphics/VertexBuffer.h>

#include <Rendering/Renderer.h>
#include <Rendering/Graphics/RenderTarget.h>

void ForwardRenderStage::Initialize()
{
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
}

void ForwardRenderStage::Update(const RenderStageData& renderData)
{
	OPTICK_EVENT();
	
	Rendering::Renderer::UploadDataToBuffer(m_vertexShader->GetCameraDataBuffer(), renderData.m_camera);

	std::unique_ptr<LightObject>& lightObject = (*renderData.m_lights)[0];
	Rendering::Renderer::UploadDataToBuffer(lightObject->GetPropertyBuffer(), &lightObject->GetData());
}

void ForwardRenderStage::Render(const RenderStageData& renderData)
{
	OPTICK_EVENT();

	Rendering::Renderer::Begin("ForwardRenderStage");
	
	const RenderEngine& renderEngine = RZE().GetRenderEngine();
	const Rendering::RenderTargetTexture& renderTarget = RZE().GetRenderEngine().GetRenderTarget();

	Rendering::Renderer::SetRenderTarget(&renderTarget);
	Rendering::Renderer::ClearRenderTarget(renderTarget.GetTargetPlatformObject(), Vector4D(0.25f, 0.25f, 0.35f, 1.0f));
	Rendering::Renderer::ClearDepthStencilBuffer(renderTarget.GetDepthTexturePlatformObject());

	Rendering::Renderer::SetVertexShader(m_vertexShader->GetPlatformObject());
	Rendering::Renderer::SetConstantBufferVS(m_vertexShader->GetCameraDataBuffer(), 0);

	Rendering::Renderer::SetViewport({ renderEngine.GetViewportSize().X(), renderEngine.GetViewportSize().Y(), 0.0f, 1.0f, 0.0f, 0.0f});

	Rendering::Renderer::SetInputLayout(m_vertexShader->GetPlatformObject());
	Rendering::Renderer::SetPrimitiveTopology(Rendering::EPrimitiveTopology::TriangleList);

	for (const auto& renderObject : *renderData.m_renderObjects)
	{
		// @note this sends in the address of renderObject->m_matrixMem.transform but fulfils the memory of struct MatrixMem as a whole
		// this should be re-evaluated later to have a better solve for the issue of commands needing access to the data being uploaded's lifetime
		Rendering::Renderer::UploadDataToBuffer(m_vertexShader->GetWorldMatrixBuffer(), &renderObject->GetTransform());
		Rendering::Renderer::SetConstantBufferVS(m_vertexShader->GetWorldMatrixBuffer(), 1);

		// @TODO
		// Currently each MeshGeometry is a draw call. Need to batch this down so it becomes a single draw call
		// per render object, at least. Can do this maybe in the burner?
		for (const auto& meshGeometry : renderObject->GetStaticMesh().GetSubMeshes())
		{
			// @TODO
			// This is god awful. Just in place while developing shader model.
			// Should get resolved once the system matures
			std::shared_ptr<const MaterialInstance> materialInstance = meshGeometry.GetMaterial();
			const PixelShader* const pixelShader = RZE().GetResourceHandler().GetResource<PixelShader>(materialInstance->GetShaderResource());

			Rendering::Renderer::SetPixelShader(pixelShader->GetPlatformObject());
			Rendering::Renderer::SetConstantBufferPS(materialInstance->GetParamBuffer(), 1);
			Rendering::Renderer::SetConstantBufferPS((*renderData.m_lights)[0]->GetPropertyBuffer(), 2);
			
			// @TODO Really need to get to texture infrastructure refactor soon - 2/6/2022
			for (U8 textureSlot = 0; textureSlot < MaterialInstance::TextureSlot::TEXTURE_SLOT_COUNT; ++textureSlot)
			{
				const ResourceHandle& resourceHandle = materialInstance->GetTexture(textureSlot);
				if (resourceHandle.IsValid())
				{
					const Texture2D* const texture = RZE().GetResourceHandler().GetResource<Texture2D>(resourceHandle);

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
