#pragma once

#include <Graphics/MaterialValues.h>

#include <RapidJSON/document.h>
#include <RapidJSON/prettywriter.h>
#include <RapidJSON/stringbuffer.h>

#include <array>
#include <map>
#include <optional>
#include <string>

// What one object changes about a material: which material asset it draws with, and each value that differs from
// that material's (ApplyTo), so the rest still follow the material
struct MaterialOverrides
{
	// A material asset drawn instead of the mesh's own; the values below go on top of it. Not a value, so ApplyTo
	// leaves it to whoever picks the material (MaterialSlot).
	std::optional<Filepath> Material;
	std::optional<MaterialValues::EBlendMode> BlendMode;
	std::optional<float> Opacity;
	std::optional<float> Shininess;
	// Indexed by MaterialInstance::TextureSlot. An overridden slot set to an invalid (default) Filepath is emptied.
	std::array<std::optional<Filepath>, MaterialValues::k_textureSlotCount> Textures;
	std::optional<bool> InvertOpacityMask;
	std::optional<bool> OpacityMaskInAlpha;

	// base, with the value overrides on top
	MaterialValues ApplyTo(const MaterialValues& base) const;
	// Nothing is overridden
	bool IsEmpty() const;
	// Some of the material's values are overridden, rather than only which material it is
	bool ChangesValues() const;

	// As a JSON object with just the overridden fields: the material by path, the blend mode by name, textures by
	// slot name (an emptied slot as ""), as scene files keep them
	void Serialize(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const;
	// What Serialize wrote; fields it doesn't recognise are ignored
	static MaterialOverrides Deserialize(const rapidjson::Value& data);
};

// One object's overrides for each of a mesh's material slots, by slot name. Ordered, so files list them the same way
// every time.
using MaterialOverridesByName = std::map<std::string, MaterialOverrides>;
