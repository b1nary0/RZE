#pragma once

#include <Graphics/GraphicsLayouts.h>
#include <Graphics/RenderData/SceneLightingData.h>
#include <Graphics/RenderStage.h>

#include <EngineCore/Resources/ResourceHandler.h>

#include <Rendering/BufferHandle.h>

#include <Utils/Math/Matrix4x4.h>
#include <Utils/Math/Vector3D.h>

class LightObject;
class RenderScene;
class VertexShader;
class PixelShader;

// Turns a scene's lights into what shading reads (SceneLightingData): a buffer of every light, uploaded each time
// it runs, and the shadow maps of the lights that cast shadows, which ForwardRenderStage samples. Each light type
// has its own kind of map:
// - The first directional light: scene depth as seen from it, in one map fitted tightly around the bounds of every
//   render object, as seen from the light. It doesn't depend on the camera, so coverage and resolution stay fixed as
//   the camera moves.
// - The first k_maxShadowedPointLights point lights: depth all around each, out to its range, in one cube of a cube
//   map array (Utils/Math/CubeMap.h).
// Each map is kept until something it shows changes, and only that map is rendered again: a point light's cube when
// the light moves or changes range, or a render object changes within its range; the directional map when its light
// turns, or any render object changes (it's fitted to them all). A map whose slot goes to another light is rendered
// again too.
class ShadowRenderStage : public IRenderStage
{
public:
	ShadowRenderStage() = default;
	~ShadowRenderStage() override = default;

public:
	const char* GetName() const override { return "ShadowRenderStage"; }

	void Initialize() override;
	void Setup(RenderStageBuilder& builder) override;
	void Render(RenderContext& context) override;

private:
	// What one shadow map was last rendered for, so it's only rendered again once that changes
	struct CachedShadowMap
	{
		static constexpr U32 k_noLight = ~0u;

		// LightObject::GetLiveID of the light it was rendered for; k_noLight until it's rendered
		U32 LightID = k_noLight;
		// RenderScene::GetRevision when it was rendered
		U64 RenderedRevision = 0;
	};

	// A scene's light buffer and shadow maps, kept in its ScenePersistent() blackboard so every view of the scene
	// shares them
	struct ShadowMapState : RenderStageData<ShadowMapState>
	{
		static constexpr const char* k_name = "ShadowMapState";

		Rendering::ConstantBufferHandle LightBuffer;
		Rendering::TextureBuffer2DHandle ShadowMap;
		// k_maxShadowedPointLights cubes, one per shadowed point light
		Rendering::TextureBuffer2DHandle PointShadowCubes;
		Rendering::ConstantBufferHandle ShadowBuffer;

		CachedShadowMap DirectionalCache;
		// By ShadowIndex, like the cubes
		CachedShadowMap PointCaches[GraphicsLayouts::k_maxShadowedPointLights];
		// What ShadowBuffer holds. Each map updates its own part when it's rendered.
		GraphicsLayouts::ShadowBufferLayout ShadowData = {};

		// RenderScene::GetRevision the maps were last brought up to date for
		U64 CheckedRevision = 0;
		bool HasCheckedMaps = false;
	};

	static ShadowMapState CreateShadowMapState();

	// Fills and uploads the scene's light buffer, which changes every frame its lights' colours or strengths do
	void UploadLights(const RenderScene& scene, const ShadowMapState& state);

	// Renders each shadowed light's map that's out of date, then uploads the ShadowBuffer that says how to read them
	// if any was
	void UpdateShadowMaps(const RenderScene& scene, ShadowMapState& state);
	// Sets up the depth-only state every light's views draw with, and starts the pass
	void BeginShadowPass();
	// Into the directional map; false if there's nothing to cast shadows
	bool RenderDirectionalShadow(const RenderScene& scene, const LightObject& light, const ShadowMapState& state, GraphicsLayouts::ShadowBufferLayout& outShadowData);
	// Into cube cubeIndex of the point light cubes, a face at a time
	void RenderPointShadow(const RenderScene& scene, const LightObject& light, U32 cubeIndex, const ShadowMapState& state, GraphicsLayouts::PointShadowLayout& outShadow);
	// Depth of every opaque caster that's in view, into the bound depth target. Shared by every light type's views.
	void DrawCasters(const RenderScene& scene, const Matrix4x4& viewProjection);

	// Returns false if there's nothing to cast shadows
	bool CalculateLightViewProjection(const RenderScene& scene, const Vector3D& lightDirection, Matrix4x4& outViewProjection, float& outWorldTexelSize) const;

private:
	RenderOutput<SceneLightingData> m_sceneLightingOutput;

	ResourceHandle m_vertexShaderResource;
	const VertexShader* m_vertexShader = nullptr;

	ResourceHandle m_casterShaderResource;
	const PixelShader* m_casterShader = nullptr;

	// Only used within one UpdateShadowMaps call, so every scene shares it
	Rendering::ConstantBufferHandle m_lightCameraBuffer;

	// So a scene with more point lights than can cast shadows says so once, not every time its maps are made
	bool m_hasWarnedOfUnshadowedPointLights = false;
};
