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

	Vector3D ChooseTint(const SourceColour& source, bool hasDiffuseTexture)
	{
		if (hasDiffuseTexture)
		{
			return Vector3D(1.0f, 1.0f, 1.0f);
		}

		const float grey = MaterialAssetImporter::k_untexturedGrey;
		Vector3D tint = source.Diffuse.value_or(Vector3D(grey, grey, grey));
		if (source.MaxTransmissionColour.has_value() && source.MaxPhysicalTransparency.has_value())
		{
			const float weight = std::clamp(*source.MaxPhysicalTransparency, 0.0f, 1.0f);
			const Vector3D& transmission = *source.MaxTransmissionColour;
			tint = Vector3D(
				tint.X() + (transmission.X() - tint.X()) * weight,
				tint.Y() + (transmission.Y() - tint.Y()) * weight,
				tint.Z() + (transmission.Z() - tint.Z()) * weight);
		}

		return Vector3D(std::clamp(tint.X(), 0.0f, 1.0f), std::clamp(tint.Y(), 0.0f, 1.0f), std::clamp(tint.Z(), 0.0f, 1.0f));
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
