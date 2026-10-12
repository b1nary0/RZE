#pragma once

#include <UI/Framework/InteractionTracker.h>

#include <Game/World/GameObject/GameObjectDefinitions.h>

struct ObjectRef;
struct PropertyDescriptor;
struct TypeDescription;

namespace Editor
{
	class EditorContext;

	// Draws a component's described properties (ComponentPropertyRegistry) as a two-column label/value table, using
	// PropertyDrawers, and turns edits into SetPropertyCommands. Nested objects and arrays are tree nodes, drawn
	// read-only. A drag or slider edit shares one interaction across the frames it's held; anything else (a checkbox,
	// Enter in a text field, a file pick) is its own interaction.
	class PropertyInspector
	{
	public:
		PropertyInspector() = default;
		~PropertyInspector() = default;

	public:
		// Around one frame's Draw calls, so a held edit's interaction can end when nothing is held any more
		void BeginFrame();
		void EndFrame();

		// False if the component's type has no described properties
		bool Draw(EditorContext& context, const GameObjectComponentPtr<GameObjectComponentBase>& component);

	private:
		// editableComponent is the component whose top-level properties these are, or null for a nested object,
		// whose properties are read-only
		void DrawProperties(EditorContext& context, const TypeDescription& type, const void* object,
			const GameObjectComponentPtr<GameObjectComponentBase>* editableComponent);
		void DrawValueRow(EditorContext& context, const PropertyDescriptor& property, size_t propertyIndex, const void* object,
			const GameObjectComponentPtr<GameObjectComponentBase>* editableComponent);
		// A tree node for a nested object, labelled heading; its properties are drawn inside when open
		void DrawObjectNode(EditorContext& context, const char* heading, const ObjectRef& ref);

		InteractionID GetInteraction(GameObjectComponentInstanceID component, size_t propertyIndex, bool isHeld);

	private:
		// The edit being held (dragged), if any, so its later frames reuse its interaction
		struct HeldEdit
		{
			GameObjectComponentInstanceID m_component = GameObjectLifetime::k_invalidID;
			size_t m_propertyIndex = 0;
			InteractionID m_interaction = k_noInteraction;
		};

		HeldEdit m_heldEdit;
		bool m_isHeldThisFrame = false;
	};
}
