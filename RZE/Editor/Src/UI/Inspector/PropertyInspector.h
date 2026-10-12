#pragma once

#include <UI/Framework/InteractionTracker.h>

#include <Game/World/GameObject/GameObjectDefinitions.h>

namespace Editor
{
	class EditorContext;

	// Draws a component's described properties (ComponentPropertyRegistry) as a two-column label/value table, using
	// PropertyDrawers, and turns edits into SetPropertyCommands. A drag or slider edit shares one interaction across
	// the frames it's held; anything else (a checkbox, Enter in a text field, a file pick) is its own interaction.
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
