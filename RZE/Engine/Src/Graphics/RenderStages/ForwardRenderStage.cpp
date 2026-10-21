#include <StdAfx.h>
#include <Graphics/RenderStages/ForwardRenderStage.h>

#include <Graphics/IndexBuffer.h>
#include <Graphics/Material.h>
#include <Graphics/MaterialSlot.h>
#include <Graphics/RenderEngine.h>
#include <Graphics/Shader.h>
#include <Graphics/Texture2D.h>
#include <Graphics/VertexBuffer.h>

#include <Rendering/Renderer.h>
#include <Rendering/Graphics/RenderTarget.h>

#include <Utils/Math/Frustum.h>
#include <Utils/Math/Units.h>

#include <algorithm>

namespace
{
	// Must match ShadowBuffer / ViewLightingBuffer / ShadowMap registers in Common/PixelResources.hlsli
	constexpr U32 k_shadowBufferSlot = 3;
	constexpr U32 k_viewLightingBufferSlot = 4;
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

	m_viewLightingBuffer = Rendering::Renderer::CreateConstantBuffer(nullptr, sizeof(ViewLightingLayout), 16, 1);
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
	const RenderScene& scene = context.Scene;
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

	LightObject* const lightObject = scene.GetLightObjects().empty() ? m_fallbackLight.get() : scene.GetLightObjects()[0].get();
	Rendering::Renderer::UploadDataToBuffer<LightObject::PropertyBufferLayout>(lightObject->GetPropertyBuffer(), &lightObject->GetData());

	ViewLightingLayout viewLighting;
	viewLighting.UniformLighting = view.Lighting == ERenderViewLighting::Uniform ? 1.0f : 0.0f;
	Rendering::Renderer::UploadDataToBuffer<ViewLightingLayout>(m_viewLightingBuffer, &viewLighting);

	Rendering::Renderer::SetVertexShader(m_vertexShader->GetPlatformObject());
	Rendering::Renderer::SetConstantBufferVS(m_vertexShader->GetCameraDataBuffer(), 0);

	Rendering::Renderer::SetViewport({ view.ViewportSize.X(), view.ViewportSize.Y(), 0.0f, 1.0f, 0.0f, 0.0f});

	Rendering::Renderer::SetInputLayout(m_vertexShader->GetPlatformObject());
	Rendering::Renderer::SetPrimitiveTopology(Rendering::EPrimitiveTopology::TriangleList);

	Rendering::Renderer::SetConstantBufferPS(shadowMapData.ShadowParams, k_shadowBufferSlot);
	Rendering::Renderer::SetTextureResource(shadowMapData.ShadowMap, k_shadowMapSlot);

	// The same for every draw: bound once, and only the world matrix buffer's contents change per object
	Rendering::Renderer::SetConstantBufferPS(lightObject->GetPropertyBuffer(), 2);
	Rendering::Renderer::SetConstantBufferPS(m_viewLightingBuffer, k_viewLightingBufferSlot);
	Rendering::Renderer::SetConstantBufferVS(m_vertexShader->GetWorldMatrixBuffer(), 1);

	// Consecutive draws often share a shader, so it's only set when it changes
	const PixelShader* boundPixelShader = nullptr;
	// Draws at the mesh's origin share the object's transform, so it's only uploaded when it changes
	const Matrix4x4* uploadedTransform = nullptr;

	// Objects and draws entirely outside the view aren't drawn
	const Frustum frustum(view.Camera.ClipSpace);

	m_transparentDraws.clear();

	for (const auto& renderObject : scene.GetRenderObjects())
	{
		if (!frustum.Intersects(renderObject->GetBounds()))
		{
			continue;
		}

		// @TODO
		// Currently each MeshDraw is a draw call. Need to batch this down so it becomes a single draw call
		// per render object, at least. Can do this maybe in the burner?
		for (const MeshDraw& draw : renderObject->GetDraws())
		{
			if (!frustum.Intersects(draw.Bounds))
			{
				continue;
			}

			// Blended draws wait until everything they could show is drawn
			if (draw.Slot->GetMaterial().IsBlended())
			{
				TransparentDraw& transparentDraw = m_transparentDraws.emplace_back();
				transparentDraw.DistanceSquared = (draw.Bounds.GetCentre() - view.Camera.Position).LengthSq();
				transparentDraw.Draw = &draw;
				continue;
			}

			DrawMesh(draw, uploadedTransform, boundPixelShader);
		}
	}

	if (!m_transparentDraws.empty())
	{
		SortBackToFront(m_transparentDraws);

		// Blended over what's behind, and tested against depth without writing it, so one see-through surface
		// doesn't hide another behind it.
		// #TODO(Transparency) Over empty background (scene alpha 0) the blend leaves alpha 0, so PostProcess paints
		// the background colour there and the surface vanishes. Needs the background handled before blending.
		Rendering::Renderer::SetDepthState(Rendering::EDepthState::ReadOnly);

		// Sorted by distance, not by mode, so the blend state is only set when it changes from the last draw's
		Rendering::EBlendState boundBlendState = Rendering::EBlendState::Opaque;
		for (const TransparentDraw& transparentDraw : m_transparentDraws)
		{
			const MeshDraw& draw = *transparentDraw.Draw;
			const Rendering::EBlendState blendState = GetBlendState(draw.Slot->GetMaterial().GetBlendMode());
			if (blendState != boundBlendState)
			{
				Rendering::Renderer::SetBlendState(blendState);
				boundBlendState = blendState;
			}

			DrawMesh(draw, uploadedTransform, boundPixelShader);
		}

		Rendering::Renderer::SetBlendState(Rendering::EBlendState::Opaque);
		Rendering::Renderer::SetDepthState(Rendering::EDepthState::ReadWrite);
	}

	// The shadow map can't stay bound as a shader input while the next frame renders depth into it
	Rendering::Renderer::UnsetTextureResource(k_shadowMapSlot);

	m_sceneColourOutput.Publish(context, sceneColourData);

	Rendering::Renderer::End();
}

void ForwardRenderStage::SortBackToFront(std::vector<TransparentDraw>& draws)
{
	std::sort(draws.begin(), draws.end(), [](const TransparentDraw& a, const TransparentDraw& b)
		{
			return a.DistanceSquared > b.DistanceSquared;
		});
}

Rendering::EBlendState ForwardRenderStage::GetBlendState(MaterialAssetImporter::MaterialData::EBlendMode blendMode)
{
	using EBlendMode = MaterialAssetImporter::MaterialData::EBlendMode;
	switch (blendMode)
	{
	case EBlendMode::Transparent:
		return Rendering::EBlendState::AlphaBlend;
	case EBlendMode::Premultiplied:
		return Rendering::EBlendState::Premultiplied;
	case EBlendMode::Additive:
		return Rendering::EBlendState::Additive;
	case EBlendMode::Multiply:
		return Rendering::EBlendState::Multiply;
	default:
		return Rendering::EBlendState::Opaque;
	}
}

void ForwardRenderStage::DrawMesh(const MeshDraw& draw, const Matrix4x4*& uploadedTransform, const PixelShader*& boundPixelShader)
{
	const MeshGeometry& meshGeometry = *draw.Geometry;

	// @note this sends in the address of a transform but fulfils the memory of struct MatrixMem as a whole
	// this should be re-evaluated later to have a better solve for the issue of commands needing access to the data being uploaded's lifetime
	if (draw.Transform != uploadedTransform)
	{
		Rendering::Renderer::UploadDataToBuffer<Matrix4x4>(m_vertexShader->GetWorldMatrixBuffer(), draw.Transform);
		uploadedTransform = draw.Transform;
	}

	// @TODO
	// This is god awful. Just in place while developing shader model.
	// Should get resolved once the system matures
	const MaterialInstance& materialInstance = draw.Slot->GetMaterial();
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
