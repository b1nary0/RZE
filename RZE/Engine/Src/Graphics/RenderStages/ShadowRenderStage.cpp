#include <StdAfx.h>
#include <Graphics/RenderStages/ShadowRenderStage.h>

#include <Graphics/IndexBuffer.h>
#include <Graphics/Material.h>
#include <Graphics/MaterialSlot.h>
#include <Graphics/MeshGeometry.h>
#include <Graphics/RenderEngine.h>
#include <Graphics/Shader.h>
#include <Graphics/Texture2D.h>
#include <Graphics/VertexBuffer.h>

#include <Rendering/Renderer.h>
#include <Rendering/Driver/GFXBuffer.h>

#include <Utils/Math/BoundingBox.h>
#include <Utils/Math/CubeMap.h>
#include <Utils/Math/Frustum.h>
#include <Utils/Math/Math.h>

#include <algorithm>
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

	// A point light's cube faces are this many texels square
	constexpr U32 k_pointShadowSize = 512;
	// Metres from a point light to its faces' near plane. Casters closer than this don't shadow; the far plane is the
	// light's range, beyond which it lights nothing.
	constexpr float k_pointShadowNear = 0.05f;

	// Which shadow map each light casts into, given the lights in the order of RenderScene::GetLightObjects: the first
	// directional light has the directional map (0), and the first point lights a cube each (0, 1, ...), up to
	// k_maxShadowedPointLights. Everything else gets -1. UploadLights and UpdateShadowMaps both go through it, so the
	// indices the shaders read are the maps that were rendered.
	struct ShadowAssigner
	{
		Int32 Assign(const LightObject& light)
		{
			switch (light.GetType())
			{
			case ELightType::Directional:
				if (!m_hasDirectional)
				{
					m_hasDirectional = true;
					return 0;
				}
				return -1;

			case ELightType::Point:
				if (m_nextCube < static_cast<Int32>(GraphicsLayouts::k_maxShadowedPointLights))
				{
					return m_nextCube++;
				}
				return -1;
			}
			return -1;
		}

	private:
		bool m_hasDirectional = false;
		Int32 m_nextCube = 0;
	};

	// How far a point light's cube faces see: out to its range, beyond which it lights nothing, but kept past the near
	// plane for a range near zero
	float CalculatePointShadowFar(const LightObject& light)
	{
		return std::max(light.GetRange(), k_pointShadowNear * 2.0f);
	}

	// The depth a projection stores for something distance metres in front of the view
	float ProjectDepth(const Matrix4x4& projection, float distance)
	{
		// View space looks down -Z
		const Vector4D clip = projection * Vector4D(0.0f, 0.0f, -distance, 1.0f);
		return clip.Z() / clip.W();
	}
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

	// Separate from the vertex shader's own camera buffer so the forward pass's camera isn't overwritten
	m_lightCameraBuffer = Rendering::Renderer::CreateConstantBuffer(nullptr, sizeof(RenderCamera), 128, 1);
}

void ShadowRenderStage::Setup(RenderStageBuilder& builder)
{
	m_sceneLightingOutput = builder.Writes<SceneLightingData>();
}

void ShadowRenderStage::Render(RenderContext& context)
{
	OPTICK_EVENT();

	ShadowMapState& state = context.ScenePersistent().GetOrCreate<ShadowMapState>(&ShadowRenderStage::CreateShadowMapState);

	// The maps depend only on the scene and its lights, not on the view, so nothing is checked until the scene's
	// revision says something changed, and then only the maps it changed are rendered again. That also skips them
	// for every view of the scene after the first in a frame. Known gap: changing a material's opacity map at runtime
	// doesn't change the revision, so a map picks that up the next time it's rendered.
	// A view lit uniformly ignores the lights, so it doesn't need the maps.
	const RenderScene& scene = context.Scene;
	const bool needsMap = context.View.Lighting == ERenderViewLighting::Scene;
	if (needsMap && (!state.HasCheckedMaps || state.CheckedRevision != scene.GetRevision()))
	{
		UpdateShadowMaps(scene, state);
		state.CheckedRevision = scene.GetRevision();
		state.HasCheckedMaps = true;
	}

	// Every view uploads it, since colours and strengths change without changing the revision. It's small.
	UploadLights(scene, state);

	// Published even without a light: an empty light buffer, and HasShadows = 0 tells the sampling shaders to skip
	// the lookup
	SceneLightingData sceneLightingData;
	sceneLightingData.LightBuffer = state.LightBuffer;
	sceneLightingData.ShadowBuffer = state.ShadowBuffer;
	sceneLightingData.DirectionalShadowMap = state.ShadowMap;
	sceneLightingData.PointShadowCubes = state.PointShadowCubes;
	m_sceneLightingOutput.Publish(context, sceneLightingData);
}

void ShadowRenderStage::UploadLights(const RenderScene& scene, const ShadowMapState& state)
{
	ShadowAssigner shadowAssigner;

	GraphicsLayouts::LightBufferLayout lightData;
	for (const std::unique_ptr<LightObject>& lightObject : scene.GetLightObjects())
	{
		if (lightData.Count == GraphicsLayouts::k_maxLights)
		{
			break;
		}

		GraphicsLayouts::LightLayout& light = lightData.Lights[lightData.Count++];
		light.Position = lightObject->GetPosition();
		light.Range = lightObject->GetRange();
		light.Direction = lightObject->GetDirection();
		light.Type = static_cast<U32>(lightObject->GetType());
		light.Colour = lightObject->GetColour().XYZ();
		light.Strength = lightObject->GetStrength();
		light.ShadowIndex = shadowAssigner.Assign(*lightObject);
	}

	Rendering::Renderer::UploadDataToBuffer<GraphicsLayouts::LightBufferLayout>(state.LightBuffer, &lightData);
}

ShadowRenderStage::ShadowMapState ShadowRenderStage::CreateShadowMapState()
{
	Rendering::GFXTextureBufferParams shadowMapParams = { 0 };
	shadowMapParams.bIsDepthTexture = true;
	shadowMapParams.bIsShaderResource = true;
	shadowMapParams.Width = k_shadowMapSize;
	shadowMapParams.Height = k_shadowMapSize;
	shadowMapParams.MipLevels = 1;
	shadowMapParams.SampleCount = 1;

	Rendering::GFXTextureBufferParams pointShadowParams = shadowMapParams;
	pointShadowParams.Width = k_pointShadowSize;
	pointShadowParams.Height = k_pointShadowSize;
	pointShadowParams.ArraySize = static_cast<U32>(ECubeFace::Count) * GraphicsLayouts::k_maxShadowedPointLights;
	pointShadowParams.bIsCubeMap = true;

	ShadowMapState state;
	state.LightBuffer = Rendering::Renderer::CreateConstantBuffer(nullptr, sizeof(GraphicsLayouts::LightBufferLayout), 16, 1);
	state.ShadowMap = Rendering::Renderer::CreateTextureBuffer2D(shadowMapParams);
	state.PointShadowCubes = Rendering::Renderer::CreateTextureBuffer2D(pointShadowParams);
	state.ShadowBuffer = Rendering::Renderer::CreateConstantBuffer(nullptr, sizeof(GraphicsLayouts::ShadowBufferLayout), 16, 1);

	state.ShadowData.TexelSize = 1.0f / static_cast<float>(k_shadowMapSize);
	state.ShadowData.PointTexelSize = 1.0f / static_cast<float>(k_pointShadowSize);
	return state;
}

void ShadowRenderStage::UpdateShadowMaps(const RenderScene& scene, ShadowMapState& state)
{
	OPTICK_EVENT();

	// A map is out of date once its slot goes to another light, or its light's shadows move. Slots no light has any
	// more are left as they are: no light's ShadowIndex points at them, and if their light gets them back they still
	// hold what it cast, which is checked against what changed since like any other map.
	const auto isLightChanged = [](const CachedShadowMap& cache, const LightObject& light)
	{
		return cache.LightID != light.GetLiveID() || light.GetShadowRevision() > cache.RenderedRevision;
	};
	const auto markRendered = [&scene](CachedShadowMap& cache, const LightObject& light)
	{
		cache.LightID = light.GetLiveID();
		cache.RenderedRevision = scene.GetRevision();
	};

	bool hasBegun = false;
	const auto beginOnce = [this, &hasBegun]()
	{
		if (!hasBegun)
		{
			BeginShadowPass();
			hasBegun = true;
		}
	};

	// The same lights, in the same order, as UploadLights gives the shaders
	ShadowAssigner shadowAssigner;
	U32 lightCount = 0;
	for (const std::unique_ptr<LightObject>& lightObject : scene.GetLightObjects())
	{
		if (lightCount++ == GraphicsLayouts::k_maxLights)
		{
			break;
		}

		const LightObject& light = *lightObject;
		const Int32 shadowIndex = shadowAssigner.Assign(light);
		switch (light.GetType())
		{
		case ELightType::Directional:
		{
			// Fitted to every render object, so any of them changing can move it
			CachedShadowMap& cache = state.DirectionalCache;
			if (shadowIndex >= 0 && (isLightChanged(cache, light) || scene.GetObjectsRevision() > cache.RenderedRevision))
			{
				beginOnce();
				state.ShadowData.HasShadows = RenderDirectionalShadow(scene, light, state, state.ShadowData) ? 1.0f : 0.0f;
				markRendered(cache, light);
			}
			break;
		}

		case ELightType::Point:
		{
			if (shadowIndex < 0)
			{
				if (!m_hasWarnedOfUnshadowedPointLights)
				{
					RZE_LOG_WARNING_ARGS("More than %u point lights: the rest light the scene without shadows.", GraphicsLayouts::k_maxShadowedPointLights);
					m_hasWarnedOfUnshadowedPointLights = true;
				}
				break;
			}

			// Together the six faces see exactly the cube out to their far planes
			const float farDistance = CalculatePointShadowFar(light);
			const BoundingBox seen(light.GetPosition() - Vector3D(farDistance), light.GetPosition() + Vector3D(farDistance));

			CachedShadowMap& cache = state.PointCaches[shadowIndex];
			if (isLightChanged(cache, light) || scene.HaveObjectsChangedWithin(seen, cache.RenderedRevision))
			{
				beginOnce();
				RenderPointShadow(scene, light, static_cast<U32>(shadowIndex), state, state.ShadowData.PointShadows[shadowIndex]);
				markRendered(cache, light);
			}
			break;
		}
		}
	}

	if (hasBegun)
	{
		Rendering::Renderer::UploadDataToBuffer<GraphicsLayouts::ShadowBufferLayout>(state.ShadowBuffer, &state.ShadowData);
		Rendering::Renderer::End();
	}
}

void ShadowRenderStage::BeginShadowPass()
{
	Rendering::Renderer::Begin("ShadowRenderStage");

	// The same for every light's views. The forward vertex shader is reused with each view's view-projection.
	Rendering::Renderer::SetVertexShader(m_vertexShader->GetPlatformObject());
	Rendering::Renderer::SetConstantBufferVS(m_lightCameraBuffer, 0);
	Rendering::Renderer::SetPixelShader(m_casterShader->GetPlatformObject());
	Rendering::Renderer::SetRasterizerState(Rendering::ERasterizerState::ShadowCaster);
	Rendering::Renderer::SetInputLayout(m_vertexShader->GetPlatformObject());
	Rendering::Renderer::SetPrimitiveTopology(Rendering::EPrimitiveTopology::TriangleList);
	// Bound once; only its contents change per object
	Rendering::Renderer::SetConstantBufferVS(m_vertexShader->GetWorldMatrixBuffer(), 1);
}

bool ShadowRenderStage::RenderDirectionalShadow(const RenderScene& scene, const LightObject& light, const ShadowMapState& state, GraphicsLayouts::ShadowBufferLayout& outShadowData)
{
	Matrix4x4 lightViewProjection;
	float worldTexelSize = 0.0f;
	if (!CalculateLightViewProjection(scene, light.GetDirection(), lightViewProjection, worldTexelSize))
	{
		return false;
	}

	outShadowData.LightViewProjection = lightViewProjection;
	outShadowData.NormalOffset = worldTexelSize * k_normalOffsetTexels;

	const float size = static_cast<float>(k_shadowMapSize);
	Rendering::Renderer::SetViewport({ size, size });
	Rendering::Renderer::ClearDepthStencilBuffer(state.ShadowMap);
	Rendering::Renderer::SetDepthTarget(state.ShadowMap);
	DrawCasters(scene, lightViewProjection);
	return true;
}

void ShadowRenderStage::RenderPointShadow(const RenderScene& scene, const LightObject& light, U32 cubeIndex, const ShadowMapState& state, GraphicsLayouts::PointShadowLayout& outShadow)
{
	const Matrix4x4 projection = Matrix4x4::CreatePerspectiveMatrixZeroToOne(90.0f, 1.0f, k_pointShadowNear, CalculatePointShadowFar(light));

	const float size = static_cast<float>(k_pointShadowSize);
	Rendering::Renderer::SetViewport({ size, size });

	// Only this light's faces: the other cubes keep what their lights cast
	for (U32 face = 0; face < static_cast<U32>(ECubeFace::Count); ++face)
	{
		const U32 slice = cubeIndex * static_cast<U32>(ECubeFace::Count) + face;
		Rendering::Renderer::ClearDepthStencilSlice(state.PointShadowCubes, slice);
		Rendering::Renderer::SetDepthTarget(state.PointShadowCubes, slice);
		DrawCasters(scene, projection * CubeMap::CreateFaceView(light.GetPosition(), static_cast<ECubeFace>(face)));
	}

	// The projection stores DepthAtInfinity + DepthDistanceScale / distance, so two distances pin both down
	const float depthAtOne = ProjectDepth(projection, 1.0f);
	const float depthAtTwo = ProjectDepth(projection, 2.0f);
	outShadow.DepthDistanceScale = 2.0f * (depthAtOne - depthAtTwo);
	outShadow.DepthAtInfinity = depthAtOne - outShadow.DepthDistanceScale;

	// A face is 2 metres across at 1 metre from the light (90 degrees)
	outShadow.NormalOffsetPerMetre = 2.0f / size * k_normalOffsetTexels;
}

void ShadowRenderStage::DrawCasters(const RenderScene& scene, const Matrix4x4& viewProjection)
{
	// Only ClipSpace is used by the caster shaders
	RenderCamera lightCamera;
	lightCamera.ClipSpace = viewProjection;
	Rendering::Renderer::UploadDataToBuffer<RenderCamera>(m_lightCameraBuffer, &lightCamera);

	// Objects and draws entirely outside the view can't cast into it (e.g. behind a cube face)
	const Frustum frustum(viewProjection);

	for (const auto& renderObject : scene.GetRenderObjects())
	{
		if (!frustum.Intersects(renderObject->GetBounds()))
		{
			continue;
		}

		// As in ForwardRenderStage: only uploaded when the draw's transform changes
		const Matrix4x4* uploadedTransform = nullptr;

		for (const MeshDraw& draw : renderObject->GetDraws())
		{
			const MeshGeometry& meshGeometry = *draw.Geometry;
			const MaterialInstance& materialInstance = draw.Slot->GetMaterial();

			// #TODO(Transparency) See-through surfaces cast nothing for now: clear glass casting a solid shadow looks
			// worse than no shadow. Partial or coloured shadows are future work (Docs/Plans/Transparency.md).
			if (materialInstance.IsBlended() || !frustum.Intersects(draw.Bounds))
			{
				continue;
			}

			// See ForwardRenderStage: uploads the whole MatrixMem (transform + inverse)
			if (draw.Transform != uploadedTransform)
			{
				Rendering::Renderer::UploadDataToBuffer<Matrix4x4>(m_vertexShader->GetWorldMatrixBuffer(), draw.Transform);
				uploadedTransform = draw.Transform;
			}

			// Only needed for cutout casters (leaves, chains, lashes)
			Rendering::Renderer::SetConstantBufferPS(materialInstance.GetParamBuffer(), 1);

			if (const Texture2D* const opacityMap = materialInstance.GetTextureResource(MaterialInstance::TEXTURE_SLOT_OPACITY))
			{
				Rendering::Renderer::SetTextureResource(opacityMap->GetPlatformObject(), MaterialInstance::TEXTURE_SLOT_OPACITY);
			}

			const Rendering::IndexBufferHandle& indexBuffer = meshGeometry.GetIndexBuffer()->GetPlatformObject();
			Rendering::Renderer::SetVertexBuffer(meshGeometry.GetVertexBuffer()->GetPlatformObject(), 0);
			Rendering::Renderer::SetIndexBuffer(indexBuffer);

			Rendering::Renderer::DrawIndexed(indexBuffer);
		}
	}
}

bool ShadowRenderStage::CalculateLightViewProjection(const RenderScene& scene, const Vector3D& lightDirection, Matrix4x4& outViewProjection, float& outWorldTexelSize) const
{
	const BoundingBox& sceneBounds = scene.GetBounds();
	if (sceneBounds.IsEmpty())
	{
		return false;
	}

	// Look along the light from just outside the scene's bounding sphere
	const Vector3D sceneCenter = sceneBounds.GetCentre();
	const float sceneRadius = sceneBounds.GetRadius();

	const Vector3D up = (std::abs(lightDirection.Y()) > 0.99f) ? WorldAxes::Forward() : WorldAxes::Up();
	const Vector3D eye = sceneCenter - lightDirection * (sceneRadius + 1.0f);
	const Matrix4x4 view = Matrix4x4::CreateViewMatrix(eye, sceneCenter, up);

	// Tight bounds of the scene as seen from the light: the draws' transformed boxes, not their looser
	// world-axis-aligned boxes
	BoundingBox lightBounds;
	for (const auto& renderObject : scene.GetRenderObjects())
	{
		for (const MeshDraw& draw : renderObject->GetDraws())
		{
			for (const Vector3D& corner : draw.Corners)
			{
				lightBounds.Grow((view * Vector4D(corner, 1.0f)).XYZ());
			}
		}
	}
	const Vector3D& lightMin = lightBounds.GetMin();
	const Vector3D& lightMax = lightBounds.GetMax();

	const float extent = std::max(lightMax.X() - lightMin.X(), lightMax.Y() - lightMin.Y());
	const float padding = extent * (k_edgePaddingTexels / static_cast<float>(k_shadowMapSize));

	// View space looks down -Z, so distances in front of the light are -z
	const float nearDistance = std::max(0.0f, -lightMax.Z() - padding);
	const float farDistance = -lightMin.Z() + padding;

	const Matrix4x4 projection = Matrix4x4::CreateOrthoMatrixZeroToOne(
		lightMin.X() - padding, lightMax.X() + padding,
		lightMin.Y() - padding, lightMax.Y() + padding,
		nearDistance, farDistance);

	outViewProjection = projection * view;
	outWorldTexelSize = (extent + 2.0f * padding) / static_cast<float>(k_shadowMapSize);
	return true;
}
