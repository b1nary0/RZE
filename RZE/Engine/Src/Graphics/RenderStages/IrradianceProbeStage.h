#pragma once

#include <Graphics/ProbeCapture.h>
#include <Graphics/RenderStage.h>

// The scene's irradiance probes (Docs/Plans/SSAOAndGlobalIllumination.md, Part 3): a grid fitted to the scene
// (IrradianceProbeGrid), each probe capturing the surfaces around it (ProbeCapture) into the scene's capture atlas, a
// batch a frame. Captures restart when the scene's bounds change. Relighting and sampling them are still to come.
class IrradianceProbeStage : public IRenderStage
{
public:
	IrradianceProbeStage();
	~IrradianceProbeStage() override = default;

public:
	const char* GetName() const override { return "IrradianceProbeStage"; }

	void Initialize() override;
	void Setup(RenderStageBuilder& builder) override;
	void Render(RenderContext& context) override;

private:
	ProbeCapture m_capture;
};
