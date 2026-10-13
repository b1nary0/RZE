#pragma once

#include <Game/World/GameObject/Properties/PropertyDescriptor.h>

namespace Editor
{
	class EditorContext;

	// How each property type is drawn in the inspector. A drawer shows value with ImGui widgets, honouring the
	// descriptor's hints (range, step, format, widget), and writes any edit back into value. It returns true on the
	// frame value changed. Widgets should use an empty ("##") label; the inspector draws the name and sets the
	// ImGui ID scope and width. A read-only value of a widget type (bool, numbers, vectors) is drawn by the same
	// drawer inside BeginDisabled; other drawers should show isReadOnly values without editing controls.
	// Object and Array properties are drawn by the inspector itself, as tree nodes.
	namespace PropertyDrawers
	{
		using Drawer = bool (*)(EditorContext& context, const PropertyDescriptor& property, PropertyValue& value, bool isReadOnly);

		// Replaces the drawer for a type; the built-in drawers are registered on first use
		void Register(EPropertyType type, Drawer drawer);

		// Null if no drawer handles the type
		Drawer Find(EPropertyType type);

		// The size the Texture drawer scales images down to fit, keeping their aspect ratio
		constexpr float k_defaultMaxTextureSize = 256.0f;

		// The Texture drawer's image, scaled down to fit maxSize pixels (keeping its aspect ratio), or at its own size
		// if maxSize is 0; for custom inspectors that show images too
		void DrawTexture(const PropertyTexture& texture, float maxSize = k_defaultMaxTextureSize);
	}
}
