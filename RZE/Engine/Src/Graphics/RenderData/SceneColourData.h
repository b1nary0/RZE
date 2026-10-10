#pragma once

#include <Graphics/RenderData/RenderStageData.h>

#include <Rendering/BufferHandle.h>

// Linear, unclipped scene colour (RGBA16F, alpha 0 marks the background) and the scene depth.
// Written by ForwardRenderStage.
struct SceneColourData : RenderStageData<SceneColourData>
{
	static constexpr const char* k_name = "SceneColourData";

	Rendering::RenderTargetHandle Colour;
	Rendering::TextureBuffer2DHandle Depth;
};
