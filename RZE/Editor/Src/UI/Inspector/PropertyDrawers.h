#pragma once

#include <Game/World/GameObject/Properties/PropertyDescriptor.h>

namespace Editor
{
	// How each property type is drawn in the inspector. A drawer shows value with ImGui widgets, honouring the
	// descriptor's hints (range, step, format, widget), and writes any edit back into value. It returns true on the
	// frame value changed. Widgets should use an empty ("##") label; the inspector draws the name and sets the
	// ImGui ID scope and width.
	namespace PropertyDrawers
	{
		using Drawer = bool (*)(const PropertyDescriptor& property, PropertyValue& value);

		// Replaces the drawer for a type; the built-in drawers are registered on first use
		void Register(EPropertyType type, Drawer drawer);

		// Null if no drawer handles the type
		Drawer Find(EPropertyType type);
	}
}
