#include <Commands/HistoryCommands.h>

#include <UI/Framework/EditorContext.h>

namespace Editor
{
	bool UndoCommand::Execute(EditorContext& context)
	{
		return context.GetUndoHistory().Undo(context);
	}

	bool RedoCommand::Execute(EditorContext& context)
	{
		return context.GetUndoHistory().Redo(context);
	}
}
