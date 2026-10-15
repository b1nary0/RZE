#include <StdAfx.h>
#include <Asset/AssetImport/SourceMaterialRules.h>

#include <algorithm>

namespace SourceMaterialRules
{
	float ChooseOpacity(const SourceTransparency& source)
	{
		float opacity = 1.0f;
		if (source.Opacity.has_value())
		{
			opacity = *source.Opacity;
		}
		else if (source.TransparencyFactor.has_value())
		{
			opacity = 1.0f - *source.TransparencyFactor;
		}
		else if (source.MaxPhysicalTransparency.has_value())
		{
			opacity = 1.0f - *source.MaxPhysicalTransparency;
		}
		return std::clamp(opacity, 0.0f, 1.0f);
	}

	MaterialAssetImporter::MaterialData::EBlendMode ChooseBlendMode(EMaskKind mask, float opacity)
	{
		using MaterialData = MaterialAssetImporter::MaterialData;
		switch (mask)
		{
		case EMaskKind::Cutout:
			return MaterialData::EBlendMode::Cutout;
		case EMaskKind::Coverage:
			return MaterialData::EBlendMode::Transparent;
		default:
			return opacity < 1.0f ? MaterialData::EBlendMode::Transparent : MaterialData::EBlendMode::Opaque;
		}
	}
}
