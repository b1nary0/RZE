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
	std::optional<EBlendMode> BlendMode;
	std::optional<float> Opacity;
	std::optional<float> Shininess;
	std::optional<Vector3D> Tint;
	// Indexed by ETextureSlot. An overridden slot set to an invalid (default) Filepath is emptied.
	std::array<std::optional<Filepath>, TEXTURE_SLOT_COUNT> Textures;
	std::optional<bool> InvertOpacityMask;
	std::optional<bool> OpacityMaskInAlpha;

	// base, with the value overrides on top
	MaterialValues ApplyTo(const MaterialValues& base) const;
	// Nothing is overridden
	bool IsEmpty() const;
	// Some of the material's values are overridden, rather than only which material it is
	bool ChangesValues() const;
	// Some override changes what a draw binds (a texture, how it's blended, how its mask is read), so drawing with it
	// takes a material of its own. The rest (Tint, Shininess, Opacity) each draw carries itself
	// (GraphicsLayouts::InstanceMaterialLayout), so an object overriding only those still draws the material it
	// overrides, in the same batch as everything else drawing it.
	bool ChangesBindings() const;

	// As a JSON object with just the overridden fields: the material by path, the blend mode by name, the tint as
	// [r, g, b], textures by slot name (an emptied slot as ""), as scene files keep them
	void Serialize(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const;
	// What Serialize wrote; fields it doesn't recognise are ignored
	static MaterialOverrides Deserialize(const rapidjson::Value& data);
};

// One object's overrides for each of a mesh's material slots, by slot name. Ordered, so files list them the same way
// every time.
using MaterialOverridesByName = std::map<std::string, MaterialOverrides>;
