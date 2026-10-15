#pragma once

#include <Graphics/MaterialValues.h>

#include <RapidJSON/document.h>
#include <RapidJSON/prettywriter.h>
#include <RapidJSON/stringbuffer.h>

#include <array>
#include <map>
#include <optional>
#include <string>

// What one object changes about a material: each field is set only where it differs from the material it's applied
// to (ApplyTo), so the rest still follows that material
struct MaterialOverrides
{
	std::optional<MaterialValues::EBlendMode> BlendMode;
	std::optional<float> Opacity;
	std::optional<float> Shininess;
	// Indexed by MaterialInstance::TextureSlot. An overridden slot set to an invalid (default) Filepath is emptied.
	std::array<std::optional<Filepath>, MaterialValues::k_textureSlotCount> Textures;
	std::optional<bool> InvertOpacityMask;
	std::optional<bool> OpacityMaskInAlpha;

	// base, with these on top
	MaterialValues ApplyTo(const MaterialValues& base) const;
	// Nothing is overridden
	bool IsEmpty() const;

	// As a JSON object with just the overridden fields: the blend mode by name, textures by slot name (an emptied
	// slot as ""), as scene files keep them
	void Serialize(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const;
	// What Serialize wrote; fields it doesn't recognise are ignored
	static MaterialOverrides Deserialize(const rapidjson::Value& data);
};

// One object's overrides for each of a mesh's material slots, by slot name. Ordered, so files list them the same way
// every time.
using MaterialOverridesByName = std::map<std::string, MaterialOverrides>;
