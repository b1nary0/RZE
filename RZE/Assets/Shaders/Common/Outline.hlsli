#ifndef OUTLINE_HLSLI
#define OUTLINE_HLSLI

// How OutlineRenderStage's mask packs a pixel: Pixel_OutlineMask writes it, Pixel_Outline reads it.

// The nearest outlined object's outline there, as its index + 1 (OutlineRenderStage::MaskBufferLayout); 0 for none
static const uint OutlineIDBits = 0xFFu;
// Set where that object is in front of the scene, so the line beside it is drawn fully, not by OccludedOpacity
static const uint OutlineInFrontBit = 0x100u;
// The bits above: which shape that object is in (OutlineRenderStage::OutlinedObject::ShapeID), numbered across the
// view's outlines, so a line is drawn where it changes
static const uint OutlineShapeShift = 9u;

#endif // OUTLINE_HLSLI
