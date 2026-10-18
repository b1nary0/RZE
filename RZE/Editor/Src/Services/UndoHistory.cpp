#include <Services/UndoHistory.h>

#include <UI/Framework/EditorContext.h>

#include <Utils/DebugUtils/Debug.h>

#include <algorithm>

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
			m_steps.back().ChangesScene = m_steps.back().ChangesScene || record->ChangesScene();

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

		// A new change makes the undone steps unreachable, and the save point with them if it was among them. Unless
		// they don't change the scene, in which case the scene here is as it was there.
		if (m_savePoint != k_noSavePoint && m_savePoint > m_position)
		{
			m_savePoint = ChangesSceneBetween(m_position, m_savePoint) ? k_noSavePoint : m_position;
		}
		for (size_t stepIndex = m_position; stepIndex < m_steps.size(); ++stepIndex)
		{
			Drop(m_steps[stepIndex]);
		}
		m_steps.erase(m_steps.begin() + m_position, m_steps.end());

		Step step;
		step.Name = record->GetName();
		step.Interaction = interaction;
		step.Records.push_back(std::move(record));
		step.SelectionBefore = std::move(selectionBefore);
		step.SelectionAfter = std::move(selectionAfter);
		step.ChangesScene = step.Records.front()->ChangesScene();
		m_steps.push_back(std::move(step));

		if (m_steps.size() > k_maxSteps)
		{
			const bool droppedChangesScene = m_steps.front().ChangesScene;
			Drop(m_steps.front());
			m_steps.pop_front();

			// Positions count from the front. The save point is lost if it was before the dropped step, unless that step
			// didn't change the scene.
			if (m_savePoint != k_noSavePoint && m_savePoint > 0)
			{
				--m_savePoint;
			}
			else if (m_savePoint == 0 && droppedChangesScene)
			{
				m_savePoint = k_noSavePoint;
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

		if (step.ChangesScene)
		{
			RestoreSelection(context, step.SelectionBefore);
		}
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

		if (step.ChangesScene)
		{
			RestoreSelection(context, step.SelectionAfter);
		}
		return true;
	}

	void UndoHistory::MarkSaved()
	{
		CloseStep();
		m_savePoint = m_position;
	}

	bool UndoHistory::IsAtSavePoint() const
	{
		return m_savePoint != k_noSavePoint && !ChangesSceneBetween(std::min(m_savePoint, m_position), std::max(m_savePoint, m_position));
	}

	bool UndoHistory::ChangesSceneBetween(size_t first, size_t last) const
	{
		return std::any_of(m_steps.begin() + first, m_steps.begin() + last, [](const Step& step) { return step.ChangesScene; });
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
		std::deque<Step> kept;
		size_t keptBeforePosition = 0;
		for (size_t stepIndex = 0; stepIndex < m_steps.size(); ++stepIndex)
		{
			if (m_steps[stepIndex].ChangesScene)
			{
				continue;
			}

			if (stepIndex < m_position)
			{
				++keptBeforePosition;
			}
			kept.push_back(std::move(m_steps[stepIndex]));
		}

		m_steps = std::move(kept);
		m_position = keptBeforePosition;
		m_isLastStepOpen = false;
		m_savePoint = m_position;
	}
}
