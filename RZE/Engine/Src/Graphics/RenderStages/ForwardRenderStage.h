#pragma once

#include <Graphics/RenderData/SceneColourData.h>
#include <Graphics/RenderData/ShadowMapData.h>
#include <Graphics/RenderStage.h>

#include <EngineCore/Resources/ResourceHandler.h>

#include <Asset/AssetImport/MaterialAssetImporter.h>

#include <Rendering/RenderingDefines.h>

#include <vector>

class Matrix4x4;
class PixelShader;
class VertexShader;
struct MeshDraw;

class ForwardRenderStage : public IRenderStage
{
public:
	ForwardRenderStage() = default;
	~ForwardRenderStage() override = default;

public:
	const char* GetName() const override { return "ForwardRenderStage"; }

	void Initialize() override;
	void Setup(RenderStageBuilder& builder) override;
	void Render(RenderContext& context) override;

public:
	// A see-through draw, drawn after everything opaque (Docs/Plans/Transparency.md)
	struct TransparentDraw
	{
		// From the camera to the centre of the draw's world bounds
		float DistanceSquared = 0.0f;
		// One of a RenderObject's draws (RenderObject::GetDraws), which stay put for the frame
		const MeshDraw* Draw = nullptr;
	};

	// Farthest first, so nearer surfaces blend over farther ones. Per draw, not per triangle: overlapping parts of
	// one draw can still blend in the wrong order.
	static void SortBackToFront(std::vector<TransparentDraw>& draws);

	// How a blended material combines with what's behind it. Opaque for materials that aren't blended.
	static Rendering::EBlendState GetBlendState(MaterialAssetImporter::MaterialData::EBlendMode blendMode);

	// Mirrors ViewLightingBuffer in Common/PixelResources.hlsli
	struct ViewLightingLayout
	{
		float UniformLighting = 0.0f; // 1 for ERenderViewLighting::Uniform
		float _pad[3] = {}; // A cbuffer is a whole number of 16-byte registers
	};

private:
	// Binds the draw's transform, material and buffers and draws it. Only uploads the transform and sets the shader
	// when they differ from the last draw's, which the two pointers track across calls.
	void DrawMesh(const MeshDraw& draw, const Matrix4x4*& uploadedTransform, const PixelShader*& boundPixelShader);

private:
	RenderInput<ShadowMapData> m_shadowMapInput;
	RenderOutput<SceneColourData> m_sceneColourOutput;

	// @TODO temp until ShaderTechniques are properly implemented
// (could be a while)
	ResourceHandle m_vertexShaderResource;
	const VertexShader* m_vertexShader = nullptr; // @TODO This is just to avoid having to GetResource() in a loop but is more a deficiency of the Resource API

	// Zero-strength light bound when the scene has none (e.g. while a scene is streaming in).
	std::unique_ptr<LightObject> m_fallbackLight;
	// How the view being rendered is lit (ViewLightingLayout)
	Rendering::ConstantBufferHandle m_viewLightingBuffer;

	// Filled and drawn every Render; kept to reuse its memory
	std::vector<TransparentDraw> m_transparentDraws;
};