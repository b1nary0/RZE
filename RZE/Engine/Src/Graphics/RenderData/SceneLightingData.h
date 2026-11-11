#pragma once

#include <Graphics/RenderData/RenderStageData.h>

#include <Rendering/BufferHandle.h>

// A scene's lights as shading reads them: every light, and the shadow maps of the ones that cast shadows
// (LightBuffer / ShadowBuffer / ShadowMap in Common/PixelResources.hlsli).
// Written by ShadowRenderStage.
struct SceneLightingData : RenderStageData<SceneLightingData>
{
	static constexpr const char* k_name = "SceneLightingData";

	// GraphicsLayouts::LightBufferLayout (Graphics/GraphicsLayouts.h)
	Rendering::ConstantBufferHandle LightBuffer;
	// GraphicsLayouts::ShadowBufferLayout (Graphics/GraphicsLayouts.h)
	Rendering::ConstantBufferHandle ShadowBuffer;
	Rendering::TextureBuffer2DHandle DirectionalShadowMap;
	// The same with Static casters alone, as StaticPointShadowCubes
	Rendering::TextureBuffer2DHandle StaticDirectionalShadowMap;
	// A cube map array, a cube per shadowed point light (by its ShadowIndex)
	Rendering::TextureBuffer2DHandle PointShadowCubes;
	// The same with Static casters alone (ERenderMobility), for what's lit to keep, like the probes: Movable objects
	// aren't captured into them, so nor are their shadows
	Rendering::TextureBuffer2DHandle StaticPointShadowCubes;
};
