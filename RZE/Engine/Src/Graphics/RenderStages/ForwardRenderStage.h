#pragma once

#include <Graphics/MeshBatches.h>
#include <Graphics/MeshInstanceFormat.h>
#include <Graphics/RenderData/AmbientOcclusionData.h>
#include <Graphics/RenderData/SceneColourData.h>
#include <Graphics/RenderData/SceneDepthData.h>
#include <Graphics/RenderData/SceneIrradianceData.h>
#include <Graphics/RenderData/SceneLightingData.h>
#include <Graphics/RenderData/SceneReflectionData.h>
#include <Graphics/RenderStage.h>

#include <EngineCore/Resources/TypedResourceHandle.h>

#include <Rendering/RenderingDefines.h>

#include <vector>

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

private:
	// Binds the batch's material and buffers and draws its instances. Only sets the shader when it differs from the last
	// batch's, which boundPixelShader tracks across calls.
	void DrawBatch(const MeshBatches::Batch& batch, const PixelShader*& boundPixelShader);

private:
	RenderInput<SceneLightingData> m_sceneLightingInput;
	RenderInput<SceneDepthData> m_sceneDepthInput;
	RenderInput<AmbientOcclusionData> m_ambientOcclusionInput;
	RenderInput<SceneIrradianceData> m_sceneIrradianceInput;
	RenderInput<SceneReflectionData> m_sceneReflectionInput;
	RenderOutput<SceneColourData> m_sceneColourOutput;

	// @TODO temp until ShaderTechniques are properly implemented
// (could be a while)
	TypedResourceHandle<VertexShader> m_vertexShader;

	// How the view being rendered is lit (ViewLightingLayout)
	Rendering::ConstantBufferHandle m_viewLightingBuffer;

	// Filled and drawn every Render; kept to reuse its memory
	std::vector<TransparentDraw> m_transparentDraws;

	// What's drawn each Render: the opaque draws, then the blended ones farthest first
	MeshBatches m_batches{ MeshInstanceFormats::Forward() };
};