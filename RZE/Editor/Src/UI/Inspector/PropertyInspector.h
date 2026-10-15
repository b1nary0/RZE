#pragma once

#include <UI/Framework/InteractionTracker.h>

#include <Game/World/GameObject/GameObjectDefinitions.h>
#include <Game/World/GameObject/Properties/PropertyPath.h>

#include <string>
#include <unordered_map>
#include <vector>

struct ObjectRef;
struct PropertyDescriptor;
struct TypeDescription;

namespace Editor
{
	class EditorContext;

	// Draws a component's described properties (ComponentPropertyRegistry) as a two-column label/value table, using
	// PropertyDrawers, and turns edits into SetPropertyCommands. Nested objects and arrays are tree nodes; an editable
	// Array's elements (PropertyDescriptor::HasEditableElements) can be edited like the component's own properties,
	// each addressed by a PropertyPath, and other nested objects are read-only. A drag or slider edit shares one
	// interaction across the frames it's held; anything else (a checkbox, Enter in a text field, a file pick) is its
	// own interaction.
	//
	// It can edit several components of the same type at once (a multi-selection): values show the first one's,
	// marked when the others differ, and an edit goes to all of them in one interaction. A nested edit goes to each
	// component whose path still leads somewhere (e.g. that has the same material slot).
	//
	// Overridable values (PropertyDescriptor::IsOverridable) show their label in the accent colour when overridden,
	// with a button that resets them; an element's header counts its overridden values and resets them all in one
	// step. Rows sharing a Group sit under its sub-heading, and lists longer than k_filterThreshold get a filter.
	class PropertyInspector
	{
	public:
		PropertyInspector() = default;
		~PropertyInspector() = default;

	public:
		// Around one frame's Draw calls, so a held edit's interaction can end when nothing is held any more
		void BeginFrame();
		void EndFrame();

		using ComponentList = std::vector<GameObjectComponentPtr<GameObjectComponentBase>>;

		// components are all the same type; the first is the one whose values are shown. False if the type has no
		// described properties.
		bool Draw(EditorContext& context, const ComponentList& components);

	private:
		// Where the properties being drawn are, for addressing edits: the component's own (Parent null), or those of
		// the element keyed ElementKey in the Array Parent leads to
		struct PathPrefix
		{
			const PropertyPath* Parent = nullptr;
			std::string ElementKey;

			PropertyPath PathTo(size_t propertyIndex) const;
		};

		// editableComponents are the components whose properties these are (or whose element's they are, through
		// prefix), or null for a read-only nested object
		void DrawProperties(EditorContext& context, const TypeDescription& type, const void* object, const PathPrefix& prefix,
			const ComponentList* editableComponents);
		void DrawValueRow(EditorContext& context, const PropertyDescriptor& property, const PropertyPath& path, const void* object,
			const ComponentList* editableComponents);
		// A Preview property's image, made from the primary component's object at path. Making it can change the
		// object, so a read-only nested object (editableComponents null) has none to show.
		void DrawPreviewRow(const PropertyDescriptor& property, const PropertyPath& path, const ComponentList* editableComponents);
		// A tree node for a nested object, labelled heading; its properties are drawn inside when open. They're editable
		// when editableComponents isn't null, addressed through prefix.
		void DrawObjectNode(EditorContext& context, const char* heading, const ObjectRef& ref, const PathPrefix& prefix,
			const ComponentList* editableComponents);

		// Resets the type's overridable properties, through prefix, on every component, in one interaction; ones that
		// aren't overridden are left as they are
		void ResetOverridden(EditorContext& context, const TypeDescription& type, const PathPrefix& prefix, const ComponentList& editableComponents);

		InteractionID GetInteraction(GameObjectComponentInstanceID component, const PropertyPath& path, bool isHeld);

	public:
		// Lists with more elements than this get a filter
		static constexpr size_t k_filterThreshold = 8;

	private:
		// The edit being held (dragged), if any, so its later frames reuse its interaction
		struct HeldEdit
		{
			GameObjectComponentInstanceID m_component = GameObjectLifetime::k_invalidID;
			PropertyPath m_path;
			InteractionID m_interaction = k_noInteraction;
		};

		HeldEdit m_heldEdit;
		bool m_isHeldThisFrame = false;

		// Each filtered list's filter text, by the list's ImGui ID
		std::unordered_map<unsigned int, std::string> m_listFilters;
	};
}
