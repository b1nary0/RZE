#include <Services/UndoHistory.h>

#include <UI/Framework/EditorContext.h>

#include <Utils/DebugUtils/Debug.h>

namespace Editor
{
	void UndoHistory::Record(InteractionID interaction, std::unique_ptr<UndoRecord> record,
		std::vector<ObjectGUID> selectionBefore, std::vector<ObjectGUID> selectionAfter)
	{
		AssertNotNull(record);

		const bool joinsLastStep = m_isLastStepOpen && interaction != k_noInteraction && m_position == m_steps.size()
			&& !m_steps.empty() && m_steps.back().Interaction == interaction;
		if (joinsLastStep)
		{
			m_steps.back().SelectionAfter = std::move(selectionAfter);

			// The newest records are the likeliest to be the same change (the same object, a frame later)
			std::vector<std::unique_ptr<UndoRecord>>& records = m_steps.back().Records;
			for (auto it = records.rbegin(); it != records.rend(); ++it)
			{
				if ((*it)->TryMerge(*record))
				{
					return;
				}
			}
			records.push_back(std::move(record));
			return;
		}

		// A new change makes the undone steps unreachable, and the save point with them if it was among them
		for (size_t stepIndex = m_position; stepIndex < m_steps.size(); ++stepIndex)
		{
			Drop(m_steps[stepIndex]);
		}
		m_steps.erase(m_steps.begin() + m_position, m_steps.end());
		if (m_savePoint != k_noSavePoint && m_savePoint > m_position)
		{
			m_savePoint = k_noSavePoint;
		}

		Step step;
		step.Name = record->GetName();
		step.Interaction = interaction;
		step.Records.push_back(std::move(record));
		step.SelectionBefore = std::move(selectionBefore);
		step.SelectionAfter = std::move(selectionAfter);
		m_steps.push_back(std::move(step));

		if (m_steps.size() > k_maxSteps)
		{
			Drop(m_steps.front());
			m_steps.pop_front();

			// Positions count from the front; the save point is lost if it was before the dropped step
			if (m_savePoint != k_noSavePoint)
			{
				m_savePoint = m_savePoint > 0 ? m_savePoint - 1 : k_noSavePoint;
			}
		}

		m_position = m_steps.size();
		m_isLastStepOpen = true;
	}

	std::string UndoHistory::GetUndoName() const
	{
		return CanUndo() ? m_steps[m_position - 1].Name : std::string();
	}

	std::string UndoHistory::GetRedoName() const
	{
		return CanRedo() ? m_steps[m_position].Name : std::string();
	}

	bool UndoHistory::Undo(EditorContext& context)
	{
		CloseStep();
		if (!CanUndo())
		{
			return false;
		}

		--m_position;
		Step& step = m_steps[m_position];
		// Last change first, so each record sees the scene as it left it
		for (auto it = step.Records.rbegin(); it != step.Records.rend(); ++it)
		{
			(*it)->Undo(context);
		}

		RestoreSelection(context, step.SelectionBefore);
		return true;
	}

	bool UndoHistory::Redo(EditorContext& context)
	{
		CloseStep();
		if (!CanRedo())
		{
			return false;
		}

		Step& step = m_steps[m_position];
		for (std::unique_ptr<UndoRecord>& record : step.Records)
		{
			record->Redo(context);
		}
		++m_position;

		RestoreSelection(context, step.SelectionAfter);
		return true;
	}

	void UndoHistory::MarkSaved()
	{
		CloseStep();
		m_savePoint = m_position;
	}

	void UndoHistory::RestoreSelection(EditorContext& context, const std::vector<ObjectGUID>& selection)
	{
		std::vector<GameObjectPtr> gameObjects;
		for (ObjectGUID guid : selection)
		{
			const GameObjectPtr gameObject = UndoTargets::FindObject(guid);
			if (gameObject != nullptr)
			{
				gameObjects.push_back(gameObject);
			}
		}
		context.GetSelection().SetSelection(gameObjects);
	}

	void UndoHistory::Drop(Step& step)
	{
		for (std::unique_ptr<UndoRecord>& record : step.Records)
		{
			record->OnDropped();
		}
	}

	void UndoHistory::Clear()
	{
		m_steps.clear();
		m_position = 0;
		m_isLastStepOpen = false;
		m_savePoint = 0;
	}
}
