#ifndef OUTLINE_HLSLI
#define OUTLINE_HLSLI

// How OutlineRenderStage's mask packs a pixel: Pixel_OutlineMask writes it, Pixel_Outline reads it.

// The nearest outlined object's outline there, as its index + 1 (OutlineRenderStage::MaskBufferLayout); 0 for none
static const uint OutlineIDBits = 0xFFu;
// Set where that object is in front of the scene, so the line beside it is drawn fully, not by OccludedOpacity
static const uint OutlineInFrontBit = 0x100u;

#endif // OUTLINE_HLSLI
