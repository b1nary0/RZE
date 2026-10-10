#include <StdAfx.h>
#include <Graphics/RenderStages/ForwardRenderStage.h>

#include <Graphics/Frustum.h>
#include <Graphics/IndexBuffer.h>
#include <Graphics/Material.h>
#include <Graphics/RenderEngine.h>
#include <Graphics/Shader.h>
#include <Graphics/Texture2D.h>
#include <Graphics/VertexBuffer.h>

#include <Rendering/Renderer.h>
#include <Rendering/Graphics/RenderTarget.h>

#include <Utils/Math/Units.h>

namespace
{
	// Must match ShadowBuffer / ShadowMap registers in Common/PixelResources.hlsli
	constexpr U32 k_shadowBufferSlot = 3;
	constexpr U32 k_shadowMapSlot = 5;
}

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

	m_fallbackLight = std::make_unique<LightObject>();
	m_fallbackLight->Initialize();
	m_fallbackLight->SetDirection(WorldAxes::Up() * -1.0f);
	m_fallbackLight->SetColour(Vector4D(0.0f, 0.0f, 0.0f, 1.0f));
	m_fallbackLight->SetStrength(0.0f);
}

void ForwardRenderStage::Setup(RenderStageBuilder& builder)
{
	m_shadowMapInput = builder.Reads<ShadowMapData>();
	m_sceneColourOutput = builder.Writes<SceneColourData>();
}

void ForwardRenderStage::Render(RenderContext& context)
{
	OPTICK_EVENT();

	Rendering::Renderer::Begin("ForwardRenderStage");

	const RenderView& view = context.View;
	const RenderEngine::SceneData& renderData = context.Scene;
	const ShadowMapData& shadowMapData = m_shadowMapInput.Get(context);

	// Linear scene colour; PostProcessRenderStage exposes and tonemaps it into the 8-bit target.
	// Alpha 0 marks the background, which it fills with the clear colour.
	SceneColourData sceneColourData;
	sceneColourData.Colour = view.Target->GetSceneTargetPlatformObject();
	sceneColourData.Depth = view.Target->GetDepthTexturePlatformObject();

	Rendering::Renderer::SetColourTarget(sceneColourData.Colour, sceneColourData.Depth);
	Rendering::Renderer::ClearRenderTarget(sceneColourData.Colour, Vector4D(0.0f, 0.0f, 0.0f, 0.0f));
	Rendering::Renderer::ClearDepthStencilBuffer(sceneColourData.Depth);

	Rendering::Renderer::UploadDataToBuffer<RenderCamera>(m_vertexShader->GetCameraDataBuffer(), &view.Camera);

	LightObject* const lightObject = renderData.lightObjects.empty() ? m_fallbackLight.get() : renderData.lightObjects[0].get();
	Rendering::Renderer::UploadDataToBuffer<LightObject::PropertyBufferLayout>(lightObject->GetPropertyBuffer(), &lightObject->GetData());

	Rendering::Renderer::SetVertexShader(m_vertexShader->GetPlatformObject());
	Rendering::Renderer::SetConstantBufferVS(m_vertexShader->GetCameraDataBuffer(), 0);

	Rendering::Renderer::SetViewport({ view.ViewportSize.X(), view.ViewportSize.Y(), 0.0f, 1.0f, 0.0f, 0.0f});

	Rendering::Renderer::SetInputLayout(m_vertexShader->GetPlatformObject());
	Rendering::Renderer::SetPrimitiveTopology(Rendering::EPrimitiveTopology::TriangleList);

	Rendering::Renderer::SetConstantBufferPS(shadowMapData.ShadowParams, k_shadowBufferSlot);
	Rendering::Renderer::SetTextureResource(shadowMapData.ShadowMap, k_shadowMapSlot);

	// The same for every draw: bound once, and only the world matrix buffer's contents change per object
	Rendering::Renderer::SetConstantBufferPS(lightObject->GetPropertyBuffer(), 2);
	Rendering::Renderer::SetConstantBufferVS(m_vertexShader->GetWorldMatrixBuffer(), 1);

	// Consecutive submeshes often share a shader, so it's only set when it changes
	const PixelShader* boundPixelShader = nullptr;

	// Objects and submeshes entirely outside the view aren't drawn
	const Frustum frustum(view.Camera.ClipSpace);

	for (const auto& renderObject : renderData.renderObjects)
	{
		if (!frustum.Intersects(renderObject->GetBoundsMin(), renderObject->GetBoundsMax()))
		{
			continue;
		}

		// @note this sends in the address of renderObject->m_matrixMem.transform but fulfils the memory of struct MatrixMem as a whole
		// this should be re-evaluated later to have a better solve for the issue of commands needing access to the data being uploaded's lifetime
		Rendering::Renderer::UploadDataToBuffer<Matrix4x4>(m_vertexShader->GetWorldMatrixBuffer(), &renderObject->GetTransform());

		const std::vector<MeshGeometry>& subMeshes = renderObject->GetStaticMesh().GetSubMeshes();
		const std::vector<RenderObject::SubMeshBounds>& subMeshBounds = renderObject->GetSubMeshBounds();

		// @TODO
		// Currently each MeshGeometry is a draw call. Need to batch this down so it becomes a single draw call
		// per render object, at least. Can do this maybe in the burner?
		for (size_t subMeshIndex = 0; subMeshIndex < subMeshes.size(); ++subMeshIndex)
		{
			if (!frustum.Intersects(subMeshBounds[subMeshIndex].Min, subMeshBounds[subMeshIndex].Max))
			{
				continue;
			}

			const MeshGeometry& meshGeometry = subMeshes[subMeshIndex];

			// @TODO
			// This is god awful. Just in place while developing shader model.
			// Should get resolved once the system matures
			const MaterialInstance& materialInstance = meshGeometry.GetMaterialRef();
			const PixelShader* const pixelShader = materialInstance.GetPixelShader();

			if (pixelShader != boundPixelShader)
			{
				Rendering::Renderer::SetPixelShader(pixelShader->GetPlatformObject());
				boundPixelShader = pixelShader;
			}
			Rendering::Renderer::SetConstantBufferPS(materialInstance.GetParamBuffer(), 1);

			// @TODO Really need to get to texture infrastructure refactor soon - 2/6/2022
			for (U8 textureSlot = 0; textureSlot < MaterialInstance::TextureSlot::TEXTURE_SLOT_COUNT; ++textureSlot)
			{
				if (const Texture2D* const texture = materialInstance.GetTextureResource(textureSlot))
				{
					// @TODO Should solve this better by providing an API that will provide a texture resource array
					Rendering::Renderer::SetTextureResource(texture->GetPlatformObject(), textureSlot);
				}
			}

			const Rendering::IndexBufferHandle& indexBuffer = meshGeometry.GetIndexBuffer()->GetPlatformObject();
			Rendering::Renderer::SetVertexBuffer(meshGeometry.GetVertexBuffer()->GetPlatformObject(), 0);
			Rendering::Renderer::SetIndexBuffer(indexBuffer);

			Rendering::Renderer::DrawIndexed(indexBuffer);
		}
	}

	// The shadow map can't stay bound as a shader input while the next frame renders depth into it
	Rendering::Renderer::UnsetTextureResource(k_shadowMapSlot);

	m_sceneColourOutput.Publish(context, sceneColourData);

	Rendering::Renderer::End();
}
