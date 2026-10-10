#include <StdAfx.h>
#include <Graphics/RenderStages/ShadowRenderStage.h>

#include <Graphics/IndexBuffer.h>
#include <Graphics/Material.h>
#include <Graphics/MeshGeometry.h>
#include <Graphics/RenderEngine.h>
#include <Graphics/Shader.h>
#include <Graphics/Texture2D.h>
#include <Graphics/VertexBuffer.h>

#include <Rendering/Renderer.h>
#include <Rendering/Driver/GFXBuffer.h>

#include <GLM/common.hpp>
#include <GLM/geometric.hpp>

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <vector>

namespace
{
	// The map covers the whole scene, so resolution = scene size / this
	constexpr U32 k_shadowMapSize = 4096;
	// Lookup offset along the surface normal, in shadow map texels. Complements the rasterizer depth bias.
	constexpr float k_normalOffsetTexels = 1.5f;
	// Border kept around the scene so PCF taps at its edge stay inside the map
	constexpr float k_edgePaddingTexels = 4.0f;
}

void ShadowRenderStage::Initialize()
{
	// Same layout as ForwardRenderStage; the forward vertex shader is reused with the light's view-projection
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

	m_casterShaderResource = RZE().GetResourceHandler().LoadResource<PixelShader>(Filepath("Assets/Shaders/Pixel_ShadowCaster.hlsl"), "Pixel_ShadowCaster");
	AssertExpr(m_casterShaderResource.IsValid());
	m_casterShader = RZE().GetResourceHandler().GetResource<PixelShader>(m_casterShaderResource);

	Rendering::GFXTextureBufferParams shadowMapParams = { 0 };
	shadowMapParams.bIsDepthTexture = true;
	shadowMapParams.bIsShaderResource = true;
	shadowMapParams.Width = k_shadowMapSize;
	shadowMapParams.Height = k_shadowMapSize;
	shadowMapParams.MipLevels = 1;
	shadowMapParams.SampleCount = 1;
	m_shadowMap = Rendering::Renderer::CreateTextureBuffer2D(nullptr, shadowMapParams);

	// Separate from the vertex shader's own camera buffer so the forward pass's camera isn't overwritten
	m_lightCameraBuffer = Rendering::Renderer::CreateConstantBuffer(nullptr, sizeof(RenderCamera), 128, 1);
	m_shadowBuffer = Rendering::Renderer::CreateConstantBuffer(nullptr, sizeof(ShadowBufferLayout), 16, 1);
}

void ShadowRenderStage::Setup(RenderStageBuilder& builder)
{
	m_shadowMapOutput = builder.Writes<ShadowMapData>();
}

void ShadowRenderStage::Render(RenderContext& context)
{
	OPTICK_EVENT();

	// The map depends only on the scene and the light, not on the view, so it's only re-rendered when
	// SceneData::revision says one of them changed. That also skips it for every view after the first in a frame.
	// Known gap: changing a material's opacity map at runtime doesn't change the revision, so the map
	// picks that up at the next scene change.
	if (!m_hasRenderedMap || m_renderedRevision != context.Scene.revision)
	{
		RenderShadowMap(context.Scene);
		m_renderedRevision = context.Scene.revision;
		m_hasRenderedMap = true;
	}

	// Published even without a light: HasShadows = 0 tells the sampling shaders to skip the lookup
	ShadowMapData shadowMapData;
	shadowMapData.ShadowMap = m_shadowMap;
	shadowMapData.ShadowParams = m_shadowBuffer;
	m_shadowMapOutput.Publish(context, shadowMapData);
}

void ShadowRenderStage::RenderShadowMap(const RenderEngine::SceneData& renderData)
{
	OPTICK_EVENT();

	Rendering::Renderer::Begin("ShadowRenderStage");

	Rendering::Renderer::SetDepthTarget(m_shadowMap);
	Rendering::Renderer::ClearDepthStencilBuffer(m_shadowMap);

	ShadowBufferLayout shadowData;
	shadowData.TexelSize = 1.0f / static_cast<float>(k_shadowMapSize);
	shadowData.NormalOffset = 0.0f;
	shadowData.HasShadows = 0.0f;
	shadowData._pad0 = 0.0f;

	Matrix4x4 lightViewProjection;
	float worldTexelSize = 0.0f;

	if (!renderData.lightObjects.empty()
		&& CalculateLightViewProjection(renderData, renderData.lightObjects[0]->GetDirection(), lightViewProjection, worldTexelSize))
	{
		// Only ClipSpace is used by the caster shaders
		RenderCamera lightCamera;
		lightCamera.ClipSpace = lightViewProjection;

		shadowData.LightViewProjection = lightCamera.ClipSpace;
		shadowData.NormalOffset = worldTexelSize * k_normalOffsetTexels;
		shadowData.HasShadows = 1.0f;

		Rendering::Renderer::UploadDataToBuffer<RenderCamera>(m_lightCameraBuffer, &lightCamera);

		Rendering::Renderer::SetVertexShader(m_vertexShader->GetPlatformObject());
		Rendering::Renderer::SetConstantBufferVS(m_lightCameraBuffer, 0);
		Rendering::Renderer::SetPixelShader(m_casterShader->GetPlatformObject());

		const float size = static_cast<float>(k_shadowMapSize);
		Rendering::Renderer::SetViewport({ size, size, 0.0f, 1.0f, 0.0f, 0.0f });
		Rendering::Renderer::SetRasterizerState(Rendering::ERasterizerState::ShadowCaster);

		Rendering::Renderer::SetInputLayout(m_vertexShader->GetPlatformObject());
		Rendering::Renderer::SetPrimitiveTopology(Rendering::EPrimitiveTopology::TriangleList);

		for (const auto& renderObject : renderData.renderObjects)
		{
			// See ForwardRenderStage: uploads the whole MatrixMem (transform + inverse)
			Rendering::Renderer::UploadDataToBuffer<Matrix4x4>(m_vertexShader->GetWorldMatrixBuffer(), &renderObject->GetTransform());
			Rendering::Renderer::SetConstantBufferVS(m_vertexShader->GetWorldMatrixBuffer(), 1);

			for (const auto& meshGeometry : renderObject->GetStaticMesh().GetSubMeshes())
			{
				std::shared_ptr<const MaterialInstance> materialInstance = meshGeometry.GetMaterial();

				// Only needed for cutout casters (leaves, chains, lashes)
				Rendering::Renderer::SetConstantBufferPS(materialInstance->GetParamBuffer(), 1);

				if (const Texture2D* const opacityMap = materialInstance->GetTextureResource(MaterialInstance::TEXTURE_SLOT_OPACITY))
				{
					Rendering::Renderer::SetTextureResource(opacityMap->GetPlatformObject(), MaterialInstance::TEXTURE_SLOT_OPACITY);
				}

				Rendering::Renderer::SetVertexBuffer(meshGeometry.GetVertexBuffer()->GetPlatformObject(), 0);
				Rendering::Renderer::SetIndexBuffer(meshGeometry.GetIndexBuffer()->GetPlatformObject());

				Rendering::Renderer::DrawIndexed(meshGeometry.GetIndexBuffer()->GetPlatformObject());
			}
		}
	}

	Rendering::Renderer::UploadDataToBuffer<ShadowBufferLayout>(m_shadowBuffer, &shadowData);

	Rendering::Renderer::End();
}

bool ShadowRenderStage::CalculateLightViewProjection(const RenderEngine::SceneData& renderData, const Vector3D& lightDirection, Matrix4x4& outViewProjection, float& outWorldTexelSize) const
{
	// World-space corners of every submesh's bounds
	std::vector<Vector3D> worldCorners;
	glm::vec3 sceneMin(FLT_MAX);
	glm::vec3 sceneMax(-FLT_MAX);

	for (const auto& renderObject : renderData.renderObjects)
	{
		const Matrix4x4& transform = renderObject->GetTransform();
		for (const MeshGeometry& meshGeometry : renderObject->GetStaticMesh().GetSubMeshes())
		{
			const Vector3D& boundsMin = meshGeometry.GetBoundsMin();
			const Vector3D& boundsMax = meshGeometry.GetBoundsMax();

			for (int corner = 0; corner < 8; ++corner)
			{
				const Vector4D local(
					(corner & 1) ? boundsMax.X() : boundsMin.X(),
					(corner & 2) ? boundsMax.Y() : boundsMin.Y(),
					(corner & 4) ? boundsMax.Z() : boundsMin.Z(),
					1.0f);
				const Vector4D world = transform * local;

				worldCorners.emplace_back(world.X(), world.Y(), world.Z());
				sceneMin = glm::min(sceneMin, glm::vec3(world.X(), world.Y(), world.Z()));
				sceneMax = glm::max(sceneMax, glm::vec3(world.X(), world.Y(), world.Z()));
			}
		}
	}

	if (worldCorners.empty())
	{
		return false;
	}

	// Look along the light from just outside the scene's bounding sphere
	const Vector3D sceneCenter((sceneMin.x + sceneMax.x) * 0.5f, (sceneMin.y + sceneMax.y) * 0.5f, (sceneMin.z + sceneMax.z) * 0.5f);
	const float sceneRadius = glm::length(sceneMax - sceneMin) * 0.5f;

	const Vector3D up = (std::abs(lightDirection.Y()) > 0.99f) ? Vector3D(0.0f, 0.0f, 1.0f) : Vector3D(0.0f, 1.0f, 0.0f);
	const Vector3D eye = sceneCenter - lightDirection * (sceneRadius + 1.0f);
	const Matrix4x4 view = Matrix4x4::CreateViewMatrix(eye, sceneCenter, up);

	// Tight bounds of the scene as seen from the light
	glm::vec3 lightMin(FLT_MAX);
	glm::vec3 lightMax(-FLT_MAX);
	for (const Vector3D& corner : worldCorners)
	{
		const Vector4D lightSpace = view * Vector4D(corner.X(), corner.Y(), corner.Z(), 1.0f);
		lightMin = glm::min(lightMin, glm::vec3(lightSpace.X(), lightSpace.Y(), lightSpace.Z()));
		lightMax = glm::max(lightMax, glm::vec3(lightSpace.X(), lightSpace.Y(), lightSpace.Z()));
	}

	const float extent = std::max(lightMax.x - lightMin.x, lightMax.y - lightMin.y);
	const float padding = extent * (k_edgePaddingTexels / static_cast<float>(k_shadowMapSize));

	// View space looks down -Z, so distances in front of the light are -z
	const float nearDistance = std::max(0.0f, -lightMax.z - padding);
	const float farDistance = -lightMin.z + padding;

	const Matrix4x4 projection = Matrix4x4::CreateOrthoMatrixZeroToOne(
		lightMin.x - padding, lightMax.x + padding,
		lightMin.y - padding, lightMax.y + padding,
		nearDistance, farDistance);

	outViewProjection = projection * view;
	outWorldTexelSize = (extent + 2.0f * padding) / static_cast<float>(k_shadowMapSize);
	return true;
}
