#pragma once

#include <Graphics/RenderData/RenderStageData.h>

#include <Rendering/BufferHandle.h>

// 8-bit colour, exposed, tonemapped and sRGB-encoded but not yet anti-aliased, and the scene depth.
// Written by PostProcessRenderStage, read by AntiAliasingStage. With the view's anti-aliasing Off, Colour is already the
// view's target, and AntiAliasingStage passes it on as it is.
struct TonemappedColourData : RenderStageData<TonemappedColourData>
{
	static constexpr const char* k_name = "TonemappedColourData";

	Rendering::RenderTargetHandle Colour;
	Rendering::TextureBuffer2DHandle Depth;
};
