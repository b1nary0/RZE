#pragma once

#include <Graphics/RenderData/ShadowMapData.h>
#include <Graphics/RenderStage.h>

#include <EngineCore/Resources/ResourceHandler.h>

#include <Rendering/BufferHandle.h>

#include <Utils/Math/Matrix4x4.h>

class VertexShader;
class PixelShader;

// Renders scene depth from the directional light into a shadow map that ForwardRenderStage samples.
// A single map is fitted tightly around the bounds of every render object, as seen from the light.
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
	void RenderShadowMap(const RenderEngine::SceneData& renderData);

	// Returns false if there's nothing to cast shadows
	bool CalculateLightViewProjection(const RenderEngine::SceneData& renderData, const Vector3D& lightDirection, Matrix4x4& outViewProjection, float& outWorldTexelSize) const;

private:
	RenderOutput<ShadowMapData> m_shadowMapOutput;

	ResourceHandle m_vertexShaderResource;
	const VertexShader* m_vertexShader = nullptr;

	ResourceHandle m_casterShaderResource;
	const PixelShader* m_casterShader = nullptr;

	Rendering::TextureBuffer2DHandle m_shadowMap;
	Rendering::ConstantBufferHandle m_lightCameraBuffer;
	Rendering::ConstantBufferHandle m_shadowBuffer;

	// SceneData::revision the shadow map was last rendered for
	U64 m_renderedRevision = 0;
	bool m_hasRenderedMap = false;
};
