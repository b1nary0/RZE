#pragma once

#include <Graphics/RenderData/ShadowMapData.h>
#include <Graphics/RenderStage.h>

#include <EngineCore/Resources/ResourceHandler.h>

#include <Rendering/BufferHandle.h>

#include <Utils/Math/Matrix4x4.h>

class VertexShader;
class PixelShader;

// Renders scene depth from the directional light into a shadow map that ForwardRenderStage samples.
// Each scene has one map, fitted tightly around the bounds of every render object, as seen from the light.
// It doesn't depend on the camera, so coverage and resolution stay fixed as the camera moves, and it's
// only re-rendered when the scene or the light changes.
class ShadowRenderStage : public IRenderStage
{
public:
	// Mirrors ShadowBuffer in Common/PixelResources.hlsli
	struct ShadowBufferLayout
	{
		Matrix4x4 LightViewProjection;
		float TexelSize;     // 1 / shadow map resolution
		float NormalOffset;  // World units to push the lookup along the surface normal
		float HasShadows;    // 0 when there is no light to cast from
		float _pad0;
	};

public:
	ShadowRenderStage() = default;
	~ShadowRenderStage() override = default;

public:
	const char* GetName() const override { return "ShadowRenderStage"; }

	void Initialize() override;
	void Setup(RenderStageBuilder& builder) override;
	void Render(RenderContext& context) override;

private:
	// A scene's shadow map, kept in its ScenePersistent() blackboard so every view of the scene shares it
	struct ShadowMapState : RenderStageData<ShadowMapState>
	{
		static constexpr const char* k_name = "ShadowMapState";

		Rendering::TextureBuffer2DHandle ShadowMap;
		Rendering::ConstantBufferHandle ShadowBuffer;

		// RenderScene::GetRevision the map was last rendered for
		U64 RenderedRevision = 0;
		bool HasRenderedMap = false;
	};

	static ShadowMapState CreateShadowMapState();

	void RenderShadowMap(const RenderScene& scene, const ShadowMapState& state);

	// Returns false if there's nothing to cast shadows
	bool CalculateLightViewProjection(const RenderScene& scene, const Vector3D& lightDirection, Matrix4x4& outViewProjection, float& outWorldTexelSize) const;

private:
	RenderOutput<ShadowMapData> m_shadowMapOutput;

	ResourceHandle m_vertexShaderResource;
	const VertexShader* m_vertexShader = nullptr;

	ResourceHandle m_casterShaderResource;
	const PixelShader* m_casterShader = nullptr;

	// Only used within one RenderShadowMap call, so every scene shares it
	Rendering::ConstantBufferHandle m_lightCameraBuffer;
};
