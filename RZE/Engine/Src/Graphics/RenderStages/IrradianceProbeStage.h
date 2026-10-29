#pragma once

#include <Graphics/ProbeCapture.h>
#include <Graphics/RenderData/SceneIrradianceData.h>
#include <Graphics/RenderData/SceneLightingData.h>
#include <Graphics/RenderStage.h>

#include <EngineCore/Resources/TypedResourceHandle.h>

#include <Rendering/BufferHandle.h>

class ComputeShader;
struct GlobalIlluminationSettings;

// The scene's irradiance probes (Docs/Plans/SSAOAndGlobalIllumination.md, Part 3): a grid fitted to the scene
// (IrradianceProbeGrid). Each frame, within its budgets (GlobalIlluminationSettings):
//   - Probes waiting to be captured are, nearest the camera first (ProbeCapture), each with a visibility map of how far
//     it sees in each direction (Compute_ProbeVisibility). They wait when the grid is fitted, and when Static objects
//     within a few spacings of them change, once those have stayed still for a few frames. The visibility pass also
//     says whether each lights anything; read back a few frames later, the scheduler stops relighting those that don't.
//   - Probes are relit with the scene's lights and last frame's probes (Compute_ProbeRelight) into spherical harmonics:
//     those just captured, those near the camera, then the rest in turn.
// The forward pass reads them for its ambient light. The grid is fitted again when the scene's bounds move by more
// than a spacing. Which probes, and when, is IrradianceProbeScheduler's choice; this stage does the GPU work.
//
// The work is done once a frame for each scene, by whichever view renders it first; every view of the scene reads it.
// Views without global illumination (RenderView::GlobalIllumination, Uniform lighting) get none, and the hemisphere
// ambient.
class IrradianceProbeStage : public IRenderStage
{
public:
	// settings: RenderEngine's, read each frame
	explicit IrradianceProbeStage(const GlobalIlluminationSettings* settings);
	~IrradianceProbeStage() override = default;

public:
	const char* GetName() const override { return "IrradianceProbeStage"; }

	void Initialize() override;
	void Setup(RenderStageBuilder& builder) override;
	void Render(RenderContext& context) override;

private:
	const GlobalIlluminationSettings* m_settings;

	RenderInput<SceneLightingData> m_sceneLightingInput;
	RenderOutput<SceneIrradianceData> m_sceneIrradianceOutput;

	ProbeCapture m_capture;
	TypedResourceHandle<ComputeShader> m_relightShader;
	TypedResourceHandle<ComputeShader> m_visibilityShader;
	Rendering::ConstantBufferHandle m_probeListBuffer;
	// GraphicsLayouts::IrradianceVolumeLayout, uploaded for each view rendered
	Rendering::ConstantBufferHandle m_volumeBuffer;
	// Bound where there are no probes to read, so the forward pass always has textures of the right kind
	Rendering::TextureBuffer2DHandle m_placeholderSH;
	Rendering::TextureBuffer2DHandle m_placeholderVisibility;
};
