#pragma once

#include <Graphics/RenderStage.h>

#include <EngineCore/Resources/ResourceHandler.h>

#include <Rendering/BufferHandle.h>

#include <Utils/Math/Matrix4x4.h>

class VertexShader;
class PixelShader;

// Renders scene depth from the directional light into a shadow map that ForwardRenderStage samples.
// A single map is fitted tightly around the bounds of every render object, as seen from the light.
// It doesn't depend on the camera, so coverage and resolution stay fixed as the camera moves.
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
	void Initialize() override;
	void Update(const RenderCamera& camera, const RenderEngine::SceneData& renderData) override {}
	void Render(const RenderCamera& camera, const RenderEngine::SceneData& renderData) override;

	// Before ForwardRenderStage
	U32 GetPriority() override { return 0; }

private:
	// Returns false if there's nothing to cast shadows
	bool CalculateLightViewProjection(const RenderEngine::SceneData& renderData, const Vector3D& lightDirection, Matrix4x4& outViewProjection, float& outWorldTexelSize) const;

private:
	ResourceHandle m_vertexShaderResource;
	const VertexShader* m_vertexShader = nullptr;

	ResourceHandle m_casterShaderResource;
	const PixelShader* m_casterShader = nullptr;

	Rendering::TextureBuffer2DHandle m_shadowMap;
	Rendering::ConstantBufferHandle m_lightCameraBuffer;
	Rendering::ConstantBufferHandle m_shadowBuffer;
};
