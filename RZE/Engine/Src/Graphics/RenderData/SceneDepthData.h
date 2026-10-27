#pragma once

#include <Graphics/RenderData/RenderStageData.h>

#include <Rendering/BufferHandle.h>

// The view's depth (D24S8, readable by shaders) of everything that isn't blended, drawn before any shading.
// Written by DepthPrepassStage; ForwardRenderStage draws over it without clearing it.
struct SceneDepthData : RenderStageData<SceneDepthData>
{
	static constexpr const char* k_name = "SceneDepthData";

	Rendering::TextureBuffer2DHandle Depth;
};
