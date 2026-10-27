#pragma once

#include <Graphics/RenderData/RenderStageData.h>

#include <Rendering/BufferHandle.h>

// How much of the indirect light reaches each pixel of the view, 0 to 1 in .r (1 = nothing occludes). It covers the
// viewport, the target's top-left corner. A 1x1 white texture when the view has no ambient occlusion, so read it with
// Load at the pixel clamped to the texture's size (GetDimensions), which then reads white everywhere, and readers never
// need to check. Written by AmbientOcclusionStage.
struct AmbientOcclusionData : RenderStageData<AmbientOcclusionData>
{
	static constexpr const char* k_name = "AmbientOcclusionData";

	Rendering::TextureBuffer2DHandle Visibility;
	// 1x1 white: for passes the ambient occlusion doesn't apply to, e.g. blended surfaces, which aren't in the depth
	// it was computed from
	Rendering::TextureBuffer2DHandle Unoccluded;
};
