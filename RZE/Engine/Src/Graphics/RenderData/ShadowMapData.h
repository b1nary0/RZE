#pragma once

#include <Graphics/RenderData/RenderStageData.h>

#include <Rendering/BufferHandle.h>

// Directional light shadow map and the parameters needed to sample it
// (ShadowMap / ShadowBuffer in Common/PixelResources.hlsli).
// Written by ShadowRenderStage.
struct ShadowMapData : RenderStageData<ShadowMapData>
{
	static constexpr const char* k_name = "ShadowMapData";

	Rendering::TextureBuffer2DHandle ShadowMap;
	Rendering::ConstantBufferHandle ShadowParams;
};
