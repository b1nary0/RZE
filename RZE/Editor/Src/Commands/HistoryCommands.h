#pragma once

#include <Commands/EditorCommand.h>

namespace Editor
{
	// Undo and redo go through the queue like any change, so they apply at the same point in the frame, in order
	// with the commands around them. They aren't recorded themselves.

	class UndoCommand : public EditorCommand
	{
	public:
		bool Execute(EditorContext& context) override;
		const char* GetName() const override { return "Undo"; }
		bool IsHistoryNavigation() const override { return true; }
	};

	class RedoCommand : public EditorCommand
	{
	public:
		bool Execute(EditorContext& context) override;
		const char* GetName() const override { return "Redo"; }
		bool IsHistoryNavigation() const override { return true; }
	};
}
