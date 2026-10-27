// The ambient occlusion alone, as grey (PostProcessRenderStage, for RenderView::Debug's AmbientOcclusion): white where
// nothing occludes.
#include "Common/PostProcess.hlsli"

Texture2D<float> AmbientOcclusion : register(t0);

float4 PSMain(PS_IN input) : SV_TARGET
{
	// Clamped, so the 1x1 white texture of a view without ambient occlusion reads white everywhere
	uint2 size;
	AmbientOcclusion.GetDimensions(size.x, size.y);
	const float visibility = AmbientOcclusion.Load(int3(min(uint2(input.Position.xy), size - 1), 0));
	return float4(visibility.xxx, 1.0f);
}
