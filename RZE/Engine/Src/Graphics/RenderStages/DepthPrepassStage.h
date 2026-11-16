#pragma once

#include <Graphics/DepthOnlyMeshBatches.h>
#include <Graphics/RenderData/SceneDepthData.h>
#include <Graphics/RenderStage.h>

#include <Rendering/BufferHandle.h>

// Draws the depth of everything that isn't blended before any shading, so the forward pass shades each pixel once,
// by its nearest surface, and so later passes (ambient occlusion) can read the scene's depth before lighting it.
// Opaque draws run no pixel shader; cutouts run one that only clips (DepthOnlyMeshBatches;
// Docs/Plans/SSAOAndGlobalIllumination.md).
class DepthPrepassStage : public IRenderStage
{
public:
	DepthPrepassStage() = default;
	~DepthPrepassStage() override = default;

public:
	const char* GetName() const override { return "DepthPrepassStage"; }

	void Initialize() override;
	void Setup(RenderStageBuilder& builder) override;
	void Render(RenderContext& context) override;

private:
	RenderOutput<SceneDepthData> m_sceneDepthOutput;

	// The camera DepthOnlyMeshBatches' vertex shaders read: computes the same depths as the forward pass's
	Rendering::ConstantBufferHandle m_cameraBuffer;

	// What's drawn each Render
	DepthOnlyMeshBatches m_batches;
};
