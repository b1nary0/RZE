#pragma once

#include <Graphics/RenderData/RenderStageData.h>

#include <Rendering/BufferHandle.h>

// A scene's lights as shading reads them: every light, and the shadow maps of the ones that cast shadows
// (LightBuffer / ShadowBuffer / ShadowMap in Common/PixelResources.hlsli).
// Written by ShadowRenderStage.
struct SceneLightingData : RenderStageData<SceneLightingData>
{
	static constexpr const char* k_name = "SceneLightingData";

	// LightBufferLayout (Graphics/GraphicsLayouts.h)
	Rendering::ConstantBufferHandle LightBuffer;
	// ShadowBufferLayout (Graphics/GraphicsLayouts.h)
	Rendering::ConstantBufferHandle ShadowBuffer;
	Rendering::TextureBuffer2DHandle DirectionalShadowMap;
	// A cube map array, a cube per shadowed point light (by its ShadowIndex)
	Rendering::TextureBuffer2DHandle PointShadowCubes;
};
