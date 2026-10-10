#pragma once

#include <Graphics/RenderData/RenderStageData.h>

#include <Rendering/BufferHandle.h>

// 8-bit, display-ready colour (exposed, tonemapped and sRGB-encoded) and the scene depth,
// so later stages can depth-test overlays against the scene.
// Written by PostProcessRenderStage; overlay stages modify it.
struct DisplayColourData : RenderStageData<DisplayColourData>
{
	static constexpr const char* k_name = "DisplayColourData";

	Rendering::RenderTargetHandle Colour;
	Rendering::TextureBuffer2DHandle Depth;
};
