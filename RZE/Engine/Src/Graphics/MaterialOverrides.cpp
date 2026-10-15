#include <StdAfx.h>
#include <Graphics/MaterialOverrides.h>

#include <Graphics/Material.h>

#include <cstring>

namespace
{
	using EBlendMode = MaterialValues::EBlendMode;

	constexpr char k_materialKey[] = "Material";
	constexpr char k_blendModeKey[] = "BlendMode";
	constexpr char k_opacityKey[] = "Opacity";
	constexpr char k_shininessKey[] = "Shininess";
	constexpr char k_tintKey[] = "Tint";
	constexpr char k_texturesKey[] = "Textures";
	constexpr char k_invertOpacityMaskKey[] = "InvertOpacityMask";
	constexpr char k_opacityMaskInAlphaKey[] = "OpacityMaskInAlpha";

	// The blend mode GetBlendModeName calls name, if there is one
	std::optional<EBlendMode> FindBlendMode(const char* name)
	{
		for (U8 mode = 0;; ++mode)
		{
			const char* const modeName = MaterialAssetImporter::GetBlendModeName(static_cast<EBlendMode>(mode));
			if (std::strcmp(modeName, "Unknown") == 0)
			{
				return std::nullopt;
			}
			if (std::strcmp(modeName, name) == 0)
			{
				return static_cast<EBlendMode>(mode);
			}
		}
	}

	// The texture slot GetTextureSlotName calls name, if there is one
	std::optional<size_t> FindTextureSlot(const char* name)
	{
		for (U8 slot = 0; slot < MaterialInstance::TEXTURE_SLOT_COUNT; ++slot)
		{
			if (std::strcmp(MaterialInstance::GetTextureSlotName(static_cast<MaterialInstance::TextureSlot>(slot)), name) == 0)
			{
				return slot;
			}
		}
		return std::nullopt;
	}
}

MaterialValues MaterialOverrides::ApplyTo(const MaterialValues& base) const
{
	MaterialValues values = base;
	values.BlendMode = BlendMode.value_or(base.BlendMode);
	values.Opacity = Opacity.value_or(base.Opacity);
	values.Shininess = Shininess.value_or(base.Shininess);
	values.Tint = Tint.value_or(base.Tint);
	for (size_t slot = 0; slot < Textures.size(); ++slot)
	{
		if (Textures[slot].has_value())
		{
			values.Textures[slot] = *Textures[slot];
		}
	}
	values.InvertOpacityMask = InvertOpacityMask.value_or(base.InvertOpacityMask);
	values.OpacityMaskInAlpha = OpacityMaskInAlpha.value_or(base.OpacityMaskInAlpha);
	return values;
}

bool MaterialOverrides::IsEmpty() const
{
	return !Material && !ChangesValues();
}

bool MaterialOverrides::ChangesValues() const
{
	for (const std::optional<Filepath>& texture : Textures)
	{
		if (texture.has_value())
		{
			return true;
		}
	}
	return BlendMode || Opacity || Shininess || Tint || InvertOpacityMask || OpacityMaskInAlpha;
}

void MaterialOverrides::Serialize(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const
{
	writer.StartObject();

	if (Material)
	{
		writer.Key(k_materialKey);
		writer.String(Material->GetRelativePath().c_str());
	}
	if (BlendMode)
	{
		writer.Key(k_blendModeKey);
		writer.String(MaterialAssetImporter::GetBlendModeName(*BlendMode));
	}
	if (Opacity)
	{
		writer.Key(k_opacityKey);
		writer.Double(*Opacity);
	}
	if (Shininess)
	{
		writer.Key(k_shininessKey);
		writer.Double(*Shininess);
	}
	if (Tint)
	{
		writer.Key(k_tintKey);
		writer.StartArray();
		writer.Double(Tint->X());
		writer.Double(Tint->Y());
		writer.Double(Tint->Z());
		writer.EndArray();
	}

	bool hasTextures = false;
	for (const std::optional<Filepath>& texture : Textures)
	{
		hasTextures = hasTextures || texture.has_value();
	}
	if (hasTextures)
	{
		writer.Key(k_texturesKey);
		writer.StartObject();
		for (U8 slot = 0; slot < Textures.size(); ++slot)
		{
			if (Textures[slot].has_value())
			{
				writer.Key(MaterialInstance::GetTextureSlotName(static_cast<MaterialInstance::TextureSlot>(slot)));
				writer.String(Textures[slot]->IsValid() ? Textures[slot]->GetRelativePath().c_str() : "");
			}
		}
		writer.EndObject();
	}

	if (InvertOpacityMask)
	{
		writer.Key(k_invertOpacityMaskKey);
		writer.Bool(*InvertOpacityMask);
	}
	if (OpacityMaskInAlpha)
	{
		writer.Key(k_opacityMaskInAlphaKey);
		writer.Bool(*OpacityMaskInAlpha);
	}

	writer.EndObject();
}

MaterialOverrides MaterialOverrides::Deserialize(const rapidjson::Value& data)
{
	MaterialOverrides overrides;
	if (!data.IsObject())
	{
		return overrides;
	}

	const auto member = [&data](const char* key) -> const rapidjson::Value*
		{
			const rapidjson::Value::ConstMemberIterator it = data.FindMember(key);
			return it != data.MemberEnd() ? &it->value : nullptr;
		};

	// A material has to be one: "" isn't kept, since Filepath("") would resolve to the root folder
	if (const rapidjson::Value* const material = member(k_materialKey); material != nullptr && material->IsString() && material->GetStringLength() > 0)
	{
		overrides.Material = Filepath(material->GetString());
	}
	if (const rapidjson::Value* const blendMode = member(k_blendModeKey); blendMode != nullptr && blendMode->IsString())
	{
		overrides.BlendMode = FindBlendMode(blendMode->GetString());
	}
	if (const rapidjson::Value* const opacity = member(k_opacityKey); opacity != nullptr && opacity->IsNumber())
	{
		overrides.Opacity = opacity->GetFloat();
	}
	if (const rapidjson::Value* const shininess = member(k_shininessKey); shininess != nullptr && shininess->IsNumber())
	{
		overrides.Shininess = shininess->GetFloat();
	}
	if (const rapidjson::Value* const tint = member(k_tintKey); tint != nullptr && tint->IsArray() && tint->Size() == 3
		&& (*tint)[0].IsNumber() && (*tint)[1].IsNumber() && (*tint)[2].IsNumber())
	{
		overrides.Tint = Vector3D((*tint)[0].GetFloat(), (*tint)[1].GetFloat(), (*tint)[2].GetFloat());
	}
	if (const rapidjson::Value* const textures = member(k_texturesKey); textures != nullptr && textures->IsObject())
	{
		for (rapidjson::Value::ConstMemberIterator it = textures->MemberBegin(); it != textures->MemberEnd(); ++it)
		{
			const std::optional<size_t> slot = FindTextureSlot(it->name.GetString());
			if (slot.has_value() && it->value.IsString())
			{
				// "" empties the slot; Filepath("") would resolve to the root folder instead
				const std::string path = it->value.GetString();
				overrides.Textures[*slot] = path.empty() ? Filepath() : Filepath(path);
			}
		}
	}
	if (const rapidjson::Value* const invert = member(k_invertOpacityMaskKey); invert != nullptr && invert->IsBool())
	{
		overrides.InvertOpacityMask = invert->GetBool();
	}
	if (const rapidjson::Value* const inAlpha = member(k_opacityMaskInAlphaKey); inAlpha != nullptr && inAlpha->IsBool())
	{
		overrides.OpacityMaskInAlpha = inAlpha->GetBool();
	}

	return overrides;
}
