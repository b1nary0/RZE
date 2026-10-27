#include <StdAfx.h>
#include <Graphics/MaterialDefines.h>

bool IsBlended(EBlendMode blendMode)
{
	return blendMode != EBlendMode::Opaque && blendMode != EBlendMode::Cutout;
}

const char* GetBlendModeName(EBlendMode blendMode)
{
	switch (blendMode)
	{
	case EBlendMode::Opaque:
		return "Opaque";
	case EBlendMode::Cutout:
		return "Cutout";
	case EBlendMode::Transparent:
		return "Transparent";
	case EBlendMode::Premultiplied:
		return "Premultiplied";
	case EBlendMode::Additive:
		return "Additive";
	case EBlendMode::Multiply:
		return "Multiply";
	default:
		return "Unknown";
	}
}

Rendering::EBlendState ToBlendState(EBlendMode blendMode)
{
	switch (blendMode)
	{
	case EBlendMode::Transparent:
		return Rendering::EBlendState::AlphaBlend;
	case EBlendMode::Premultiplied:
		return Rendering::EBlendState::Premultiplied;
	case EBlendMode::Additive:
		return Rendering::EBlendState::Additive;
	case EBlendMode::Multiply:
		return Rendering::EBlendState::Multiply;
	default:
		return Rendering::EBlendState::Opaque;
	}
}

const char* GetTextureSlotName(ETextureSlot textureSlot)
{
	switch (textureSlot)
	{
	case TEXTURE_SLOT_DIFFUSE:
		return "Diffuse";
	case TEXTURE_SLOT_SPECULAR:
		return "Specular";
	case TEXTURE_SLOT_NORMAL:
		return "Normal";
	case TEXTURE_SLOT_OPACITY:
		return "Opacity";
	case TEXTURE_SLOT_HEIGHT:
		return "Height";
	default:
		return "None";
	}
}
