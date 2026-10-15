#pragma once

#include <Game/World/GameObject/Properties/PropertyDescriptor.h>

namespace Editor
{
	class EditorContext;

	// How each property type is drawn in the inspector. A drawer shows value with ImGui widgets, honouring the
	// descriptor's hints (range, step, format, widget), and writes any edit back into value. It returns true on the
	// frame value changed. object is what the property is on, for metadata that reads it (e.g. an asset's preview).
	// Widgets should use an empty ("##") label; the inspector draws the name and sets the ImGui ID scope and the item
	// width, which drawers should fit in. A read-only value of a widget type (bool, numbers, vectors, enums) is drawn
	// by the same drawer inside BeginDisabled; other drawers should show isReadOnly values without editing controls.
	// Object and Array properties are drawn by the inspector itself, as tree nodes.
	namespace PropertyDrawers
	{
		using Drawer = bool (*)(EditorContext& context, const PropertyDescriptor& property, const void* object, PropertyValue& value, bool isReadOnly);

		// Replaces the drawer for a type; the built-in drawers are registered on first use
		void Register(EPropertyType type, Drawer drawer);

		// Null if no drawer handles the type
		Drawer Find(EPropertyType type);

		// The size the Texture drawer scales images down to fit, keeping their aspect ratio
		constexpr float k_defaultMaxTextureSize = 256.0f;

		// The Texture drawer's image, scaled down to fit maxSize pixels (keeping its aspect ratio), or at its own size
		// if maxSize is 0; for custom inspectors that show images too
		void DrawTexture(const PropertyTexture& texture, float maxSize = k_defaultMaxTextureSize);

		// The size a thumbnail's hover preview scales images down to fit: large enough to see detail, small enough not
		// to cover the inspector
		constexpr float k_thumbnailPreviewSize = 320.0f;

		// texture as a size x size thumbnail (an empty frame without one, keeping its place) that shows it at up to
		// k_thumbnailPreviewSize, with its dimensions, below and to the left of the cursor while it's hovered
		void DrawThumbnail(const PropertyTexture& texture, float size);
	}
}
