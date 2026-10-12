#pragma once

#include <UI/Framework/InteractionTracker.h>

namespace Editor
{
	class EditorContext;

	// A change the editor UI asks for. Panels push commands to EditorContext's CommandQueue instead of changing
	// the scene while they're drawing, and the queue applies them once every panel has drawn. Commands hold
	// handles rather than pointers, so one whose target was destroyed in the meantime (e.g. by an earlier
	// command in the same flush) does nothing.
	class EditorCommand
	{
	public:
		explicit EditorCommand(InteractionID interaction = k_noInteraction)
			: m_interaction(interaction) {}
		virtual ~EditorCommand() = default;

		virtual void Execute(EditorContext& context) = 0;

		// For logs and, later, undo history
		virtual const char* GetName() const = 0;

		// The continuous interaction (e.g. a gizmo drag) this command is one step of, or k_noInteraction for a
		// one-off change. Undo will merge consecutive commands from the same interaction into one step.
		InteractionID GetInteraction() const { return m_interaction; }

	private:
		InteractionID m_interaction;
	};
}
