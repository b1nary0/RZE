#pragma once

#include <Rendering/RenderingDefines.h>

#include <Utils/PrimitiveDefs.h>

// How a material covers what's behind it (Docs/Plans/Transparency.md). Stored as a byte in material assets; append
// new modes.
enum class EBlendMode : U8
{
	Opaque,      // Hides what's behind it
	Cutout,      // Opaque where its opacity mask passes AlphaCutoff, absent elsewhere
	Transparent, // See-through by its opacity: src * a + dst * (1 - a)
	Premultiplied, // For colour already multiplied by its alpha: src + dst * (1 - a)
	Additive,    // Adds its colour, scaled by opacity, to what's behind (glows, light shafts): src * a + dst
	Multiply     // Darkens what's behind by its colour (grime): src * dst. #TODO(Transparency) ignores opacity
};

// The textures a material can have, which are also the texture registers its shaders read them from. Indexes
// MaterialValues::Textures.
enum ETextureSlot : U8
{
	TEXTURE_SLOT_DIFFUSE,
	TEXTURE_SLOT_SPECULAR,
	TEXTURE_SLOT_NORMAL,
	TEXTURE_SLOT_OPACITY,
	TEXTURE_SLOT_HEIGHT,
	TEXTURE_SLOT_COUNT
};

// Blends with what's behind it, so it's drawn after everything opaque, sorted, and casts no shadow
bool IsBlended(EBlendMode blendMode);
const char* GetBlendModeName(EBlendMode blendMode);
// How a blended material combines with what's behind it. Opaque for materials that aren't blended.
Rendering::EBlendState ToBlendState(EBlendMode blendMode);

const char* GetTextureSlotName(ETextureSlot textureSlot);
