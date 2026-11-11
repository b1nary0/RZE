#pragma once

#include <Graphics/GraphicsLayouts.h>
#include <Graphics/RenderData/SceneLightingData.h>
#include <Graphics/RenderStage.h>

#include <EngineCore/Resources/TypedResourceHandle.h>

#include <Rendering/BufferHandle.h>

#include <Utils/LiveIDAllocator.h>
#include <Utils/Math/BoundingBox.h>
#include <Utils/Math/Matrix4x4.h>
#include <Utils/Math/Vector3D.h>

class LightObject;
class RenderScene;
struct RenderWorkCounts;
class VertexShader;
class PixelShader;

// Turns a scene's lights into what shading reads (SceneLightingData): a buffer of every light, uploaded each time
// it runs, and the shadow maps of the lights that cast shadows, which ForwardRenderStage samples. Each light type
// has its own kind of map:
// - The first directional light: scene depth as seen from it, in one map fitted around the Static render objects
//   (ERenderMobility) as seen from the light, with some room to spare. It doesn't depend on the camera, so coverage
//   and resolution stay fixed as the camera moves, and it's only fitted again when the light turns or a Static object
//   leaves it. Movable objects never fit it, so one outside it casts no sun shadow. A second map holds the Static
//   casters alone; the published one is it copied, with the Movable casters drawn on top.
// - The first k_maxShadowedPointLights point lights: depth all around each, out to its range, in one cube of a cube
//   map array (Graphics/CubeMap.h). A second array holds Static casters alone (ERenderMobility), so a face with
//   Movable casters in it is the Static face copied, with them drawn on top.
// Each map is kept until something it shows changes, and only that map is rendered again: a point light's cube when
// the light moves or changes range (every face), or only the faces a render object changed in front of, within its
// range; the directional map when its light turns, or a render object changes within its fit. Either way a Static
// object changing redraws the Static casters, and a Movable one only copies them again before drawing the Movable
// ones. A map whose slot goes to another light is rendered again too.
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
	// Which render objects DrawCasters draws
	enum class ECasters
	{
		All,
		Static,
		Movable
	};

	// What one shadow map was last rendered for, so it's only rendered again once that changes
	struct CachedShadowMap
	{
		static constexpr U64 k_noLight = LiveIDAllocator::k_invalidID;

		// LightObject::GetLiveID of the light it was rendered for; k_noLight until it's rendered
		U64 LightID = k_noLight;
		// RenderScene::GetRevision when it was rendered
		U64 RenderedRevision = 0;
	};

	// Where the directional map looks from and what it covers, kept until the light turns or a Static object leaves it
	struct DirectionalFit
	{
		bool IsFitted = false;
		Matrix4x4 View;
		// What the map covers in the light's view space, room to spare included
		BoundingBox LightBounds;
		Matrix4x4 ViewProjection;
		float WorldTexelSize = 0.0f;
	};

	// A scene's light buffer and shadow maps, kept in its ScenePersistent() blackboard so every view of the scene
	// shares them
	struct ShadowMapState : RenderStageData<ShadowMapState>
	{
		static constexpr const char* k_name = "ShadowMapState";

		Rendering::ConstantBufferHandle LightBuffer;
		// The directional light's, and the same with Static casters alone
		Rendering::TextureBuffer2DHandle ShadowMap;
		Rendering::TextureBuffer2DHandle StaticShadowMap;
		// k_maxShadowedPointLights cubes, one per shadowed point light, and the same with Static casters alone
		Rendering::TextureBuffer2DHandle PointShadowCubes;
		Rendering::TextureBuffer2DHandle StaticPointShadowCubes;
		Rendering::ConstantBufferHandle ShadowBuffer;

		CachedShadowMap DirectionalCache;
		DirectionalFit Fit;
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
	// if any was. Counts what it renders into workCounts.
	void UpdateShadowMaps(const RenderScene& scene, ShadowMapState& state, RenderWorkCounts& workCounts);
	// Sets up the depth-only state every light's views draw with, and starts the pass
	void BeginShadowPass();
	// Into the directional map, as state.Fit has it: its Static casters again into the Static map if redrawStatic, then
	// that copied, with the Movable casters on top
	void RenderDirectionalShadow(const RenderScene& scene, bool redrawStatic, const ShadowMapState& state, RenderWorkCounts& workCounts);
	// Into cube cubeIndex of the point light cubes: the faces in staticFaces (a bit each, 1 << ECubeFace) have their
	// Static casters drawn again into the Static cube, and those and the faces in movableFaces are copied from it and
	// have their Movable casters drawn on top
	void RenderPointShadow(const RenderScene& scene, const LightObject& light, U32 cubeIndex, U32 staticFaces, U32 movableFaces, const ShadowMapState& state, GraphicsLayouts::PointShadowLayout& outShadow, RenderWorkCounts& workCounts);
	// Depth of every opaque caster of those kinds that's in view, into the bound depth target. Shared by every light
	// type's views. Returns how many draws it made.
	U32 DrawCasters(const RenderScene& scene, const Matrix4x4& viewProjection, ECasters casters);

	// Fits the directional map around the Static casters as seen along lightDirection, with room to spare; false
	// (outFit not fitted) if there are none to cast shadows
	bool FitDirectionalShadow(const RenderScene& scene, const Vector3D& lightDirection, DirectionalFit& outFit) const;

private:
	RenderOutput<SceneLightingData> m_sceneLightingOutput;

	TypedResourceHandle<VertexShader> m_vertexShader;

	TypedResourceHandle<PixelShader> m_casterShader;

	// Only used within one UpdateShadowMaps call, so every scene shares it
	Rendering::ConstantBufferHandle m_lightCameraBuffer;

	// So a scene with more point lights than can cast shadows says so once, not every time its maps are made
	bool m_hasWarnedOfUnshadowedPointLights = false;
};
