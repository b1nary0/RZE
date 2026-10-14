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
class RenderObject;
class VertexShader;

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
	// A see-through placement, drawn after everything opaque (Docs/Plans/Transparency.md)
	struct TransparentDraw
	{
		// From the camera to the centre of the placement's world bounds
		float DistanceSquared = 0.0f;
		const RenderObject* Object = nullptr;
		size_t PlacementIndex = 0;
	};

	// Farthest first, so nearer surfaces blend over farther ones. Per placement, not per triangle: overlapping
	// parts of one placement can still blend in the wrong order.
	static void SortBackToFront(std::vector<TransparentDraw>& draws);

	// How a blended material combines with what's behind it. Opaque for materials that aren't blended.
	static Rendering::EBlendState GetBlendState(MaterialAssetImporter::MaterialData::EBlendMode blendMode);

private:
	// Binds the placement's transform, material and buffers and draws it. Only uploads the transform and sets the
	// shader when they differ from the last draw's, which the two pointers track across calls.
	void DrawPlacement(const RenderObject& renderObject, size_t placementIndex, const Matrix4x4*& uploadedTransform, const PixelShader*& boundPixelShader);

private:
	RenderInput<ShadowMapData> m_shadowMapInput;
	RenderOutput<SceneColourData> m_sceneColourOutput;

	// @TODO temp until ShaderTechniques are properly implemented
// (could be a while)
	ResourceHandle m_vertexShaderResource;
	const VertexShader* m_vertexShader = nullptr; // @TODO This is just to avoid having to GetResource() in a loop but is more a deficiency of the Resource API

	// Zero-strength light bound when the scene has none (e.g. while a scene is streaming in).
	std::unique_ptr<LightObject> m_fallbackLight;

	// Filled and drawn every Render; kept to reuse its memory
	std::vector<TransparentDraw> m_transparentDraws;
};