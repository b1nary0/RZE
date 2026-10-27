#pragma once

#include <Graphics/RenderData/RenderStageData.h>

#include <Rendering/BufferHandle.h>

// The scene's reflection probes as the forward pass reads them (Common/ReflectionProbes.hlsli): their boxes
// (GraphicsLayouts::ReflectionProbeBufferLayout, b7) and their prefiltered cubes, a cube array (t12). With no probes,
// the count is 0 and the cubes are a placeholder. Written by ReflectionProbeStage.
struct SceneReflectionData : RenderStageData<SceneReflectionData>
{
	static constexpr const char* k_name = "SceneReflectionData";

	Rendering::ConstantBufferHandle ProbeBuffer;
	Rendering::TextureBuffer2DHandle Cubes;
};
