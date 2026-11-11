#pragma once

#include <Graphics/RenderData/RenderStageData.h>

#include <Rendering/BufferHandle.h>

// The scene's irradiance probes as the forward pass reads them (Common/IrradianceVolume.hlsli): where they are and how
// many can be read (GraphicsLayouts::IrradianceVolumeLayout, b5), their spherical harmonics, a texture array per colour
// channel (t8 to t10), and their visibility maps (t11). With no probes to read, ValidProbeCount is 0 and the textures
// are placeholders.
// Written by IrradianceProbeStage.
struct SceneIrradianceData : RenderStageData<SceneIrradianceData>
{
	static constexpr const char* k_name = "SceneIrradianceData";

	Rendering::ConstantBufferHandle VolumeBuffer;
	Rendering::TextureBuffer2DHandle SHRed;
	Rendering::TextureBuffer2DHandle SHGreen;
	Rendering::TextureBuffer2DHandle SHBlue;
	Rendering::TextureBuffer2DHandle Visibility;
	// Whether the probes were relit this frame, so their light is still changing (IrradianceProbeScheduler::IsSettling)
	bool IsChanging = false;
};
