#pragma once

#include <Game/World/GameObject/GameObjectDefinitions.h>
#include <Game/World/GameObject/ObjectGUID.h>
#include <Game/World/GameObject/Properties/PropertyDescriptor.h>
#include <Game/World/GameObject/Properties/PropertyPath.h>

#include <functional>
#include <string>
#include <utility>
#include <vector>

class GameObject;
class GameScene;

namespace Editor
{
	// A GameObject copied into memory as it is right now, with its children: its name, which components it has, and
	// their property values. Independent of any scene, and of the scene file format, so it holds unsaved changes and
	// can be put into another scene. What the editor copies, pastes and duplicates.
	//
	// The object's own state is handled field by field, so a new field or GameObjectStateFlags flag needs a decision
	// here: the name is copied (and numbered by the paste); IncludeInSave is copied, so an object that isn't saved
	// copies as one that isn't either; the GUID and ID are new; the parent and children are rebuilt in order; and
	// IsInScene is the scene's to set.
	class GameObjectSnapshot
	{
	public:
		using NameMaker = std::function<std::string(const std::string& copiedName)>;

	public:
		// Copies every property with a setter (see PropertyDescriptor::Set), ReadOnly ones included, except:
		// Exclusive ones, so a copy never takes over from the original (e.g. as the main camera), EditorOnly view
		// settings, and overridable values that aren't overridden (the copy has the same base). That includes the
		// properties of editable Arrays' elements, which are set after the component's own (which may be what creates
		// the elements, e.g. a mesh's material slots); an element the copy doesn't have is skipped.
		static GameObjectSnapshot Take(GameObject& gameObject);

		// Adds a new object (and its children) to scene, under parent if it isn't null, and returns it. Each object is
		// named by makeName, given the copied name. The objects get new GUIDs.
		GameObjectPtr Create(GameScene& scene, const GameObjectPtr& parent, const NameMaker& makeName) const;

		// The copied object's parent when it was taken, or an invalid GUID if it was a root object
		ObjectGUID GetParentGUID() const { return m_parentGUID; }

	private:
		struct ComponentValues
		{
			GameObjectComponentID ID;
			// The component's own properties first, then its elements', each with where it goes
			std::vector<std::pair<PropertyPath, PropertyValue>> Values;
		};

	private:
		std::string m_name;
		ObjectGUID m_parentGUID = ObjectGUIDs::k_invalid;
		bool m_includeInSave = true;
		std::vector<ComponentValues> m_components;
		std::vector<GameObjectSnapshot> m_children;
	};
}
