#pragma once

#include <Graphics/ProbeCapture.h>
#include <Graphics/RenderData/SceneIrradianceData.h>
#include <Graphics/RenderData/SceneLightingData.h>
#include <Graphics/RenderStage.h>

#include <EngineCore/Resources/TypedResourceHandle.h>

#include <Rendering/BufferHandle.h>

class ComputeShader;

// The scene's irradiance probes (Docs/Plans/SSAOAndGlobalIllumination.md, Part 3): a grid fitted to the scene
// (IrradianceProbeGrid), each probe capturing the surfaces around it (ProbeCapture) a batch a frame, with a visibility
// map of how far it sees in each direction (Compute_ProbeVisibility), then relit a batch a frame with the scene's lights
// and last frame's probes (Compute_ProbeRelight) into spherical harmonics, which the forward pass reads for its ambient
// light. Captures restart when the scene's bounds change.
//
// Publishes SceneIrradianceData in every view; only the main view captures and relights. A scene the main view hasn't
// drawn has no probes, and its views fall back to the hemisphere ambient.
class IrradianceProbeStage : public IRenderStage
{
public:
	// Mirrors RelightParams in Compute_ProbeRelight.hlsl
	struct RelightParamsLayout
	{
		U32 FirstProbe;
		U32 RelightRange;
		U32 FaceSize;
		U32 AtlasProbesPerRow;
	};

public:
	IrradianceProbeStage();
	~IrradianceProbeStage() override = default;

public:
	const char* GetName() const override { return "IrradianceProbeStage"; }

	void Initialize() override;
	void Setup(RenderStageBuilder& builder) override;
	void Render(RenderContext& context) override;

private:
	RenderInput<SceneLightingData> m_sceneLightingInput;
	RenderOutput<SceneIrradianceData> m_sceneIrradianceOutput;

	ProbeCapture m_capture;
	TypedResourceHandle<ComputeShader> m_relightShader;
	TypedResourceHandle<ComputeShader> m_visibilityShader;
	Rendering::ConstantBufferHandle m_relightParamsBuffer;
	// GraphicsLayouts::IrradianceVolumeLayout, uploaded for each view rendered
	Rendering::ConstantBufferHandle m_volumeBuffer;
	// Bound where there are no probes to read, so the forward pass always has textures of the right kind
	Rendering::TextureBuffer2DHandle m_placeholderSH;
	Rendering::TextureBuffer2DHandle m_placeholderVisibility;
};
