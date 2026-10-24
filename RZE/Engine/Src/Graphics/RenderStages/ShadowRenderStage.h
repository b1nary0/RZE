#pragma once

#include <Graphics/RenderData/SceneLightingData.h>
#include <Graphics/RenderStage.h>

#include <EngineCore/Resources/ResourceHandler.h>

#include <Rendering/BufferHandle.h>

#include <Utils/Math/Matrix4x4.h>
#include <Utils/Math/Vector3D.h>

class RenderScene;
class VertexShader;
class PixelShader;

// Turns a scene's lights into what shading reads (SceneLightingData): a buffer of every light, uploaded each time
// it runs, and the shadow maps of the lights that cast shadows.
// The first directional light casts shadows: scene depth as seen from it, rendered into a shadow map that
// ForwardRenderStage samples. Each scene has one map, fitted tightly around the bounds of every render object, as
// seen from the light. It doesn't depend on the camera, so coverage and resolution stay fixed as the camera moves,
// and it's only re-rendered when the scene or the light changes.
class ShadowRenderStage : public IRenderStage
{
public:
	// Mirrors MAX_LIGHTS in Common/PixelResources.hlsli. Lights past it are left out.
	static constexpr U32 k_maxLights = 16;

	// Mirrors Light in Common/PixelResources.hlsli. Each type reads only what it uses (LightObject).
	struct LightLayout
	{
		Vector3D Position;
		float Range = 0.0f;
		Vector3D Direction;
		U32 Type = 0; // ELightType
		Vector3D Colour; // sRGB
		float Strength = 0.0f;
		Int32 ShadowIndex = -1; // Which of its type's shadow maps it casts into; -1 for none
		float _pad[3] = {}; // A cbuffer array element is a whole number of 16-byte registers
	};

	// Mirrors LightBuffer in Common/PixelResources.hlsli
	struct LightBufferLayout
	{
		LightLayout Lights[k_maxLights];
		U32 Count = 0;
		float _pad[3] = {};
	};

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
	// A scene's light buffer and shadow map, kept in its ScenePersistent() blackboard so every view of the scene
	// shares them
	struct ShadowMapState : RenderStageData<ShadowMapState>
	{
		static constexpr const char* k_name = "ShadowMapState";

		Rendering::ConstantBufferHandle LightBuffer;
		Rendering::TextureBuffer2DHandle ShadowMap;
		Rendering::ConstantBufferHandle ShadowBuffer;

		// RenderScene::GetRevision the map was last rendered for
		U64 RenderedRevision = 0;
		bool HasRenderedMap = false;
	};

	static ShadowMapState CreateShadowMapState();

	// Fills and uploads the scene's light buffer, which changes every frame its lights' colours or strengths do
	void UploadLights(const RenderScene& scene, const ShadowMapState& state);

	void RenderShadowMap(const RenderScene& scene, const ShadowMapState& state);

	// Returns false if there's nothing to cast shadows
	bool CalculateLightViewProjection(const RenderScene& scene, const Vector3D& lightDirection, Matrix4x4& outViewProjection, float& outWorldTexelSize) const;

private:
	RenderOutput<SceneLightingData> m_sceneLightingOutput;

	ResourceHandle m_vertexShaderResource;
	const VertexShader* m_vertexShader = nullptr;

	ResourceHandle m_casterShaderResource;
	const PixelShader* m_casterShader = nullptr;

	// Only used within one RenderShadowMap call, so every scene shares it
	Rendering::ConstantBufferHandle m_lightCameraBuffer;
};
