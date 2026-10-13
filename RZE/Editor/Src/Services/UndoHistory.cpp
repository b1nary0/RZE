#include <Services/UndoHistory.h>

#include <Utils/DebugUtils/Debug.h>

namespace Editor
{
	void UndoHistory::Record(InteractionID interaction, std::unique_ptr<UndoRecord> record)
	{
		AssertNotNull(record);

		const bool joinsLastStep = m_isLastStepOpen && interaction != k_noInteraction && m_position == m_steps.size()
			&& !m_steps.empty() && m_steps.back().Interaction == interaction;
		if (joinsLastStep)
		{
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

		// A new change makes the undone steps unreachable
		m_steps.erase(m_steps.begin() + m_position, m_steps.end());

		Step step;
		step.Name = record->GetName();
		step.Interaction = interaction;
		step.Records.push_back(std::move(record));
		m_steps.push_back(std::move(step));

		if (m_steps.size() > k_maxSteps)
		{
			m_steps.pop_front();
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
		std::vector<std::unique_ptr<UndoRecord>>& records = m_steps[m_position].Records;
		// Last change first, so each record sees the scene as it left it
		for (auto it = records.rbegin(); it != records.rend(); ++it)
		{
			(*it)->Undo(context);
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

		for (std::unique_ptr<UndoRecord>& record : m_steps[m_position].Records)
		{
			record->Redo(context);
		}
		++m_position;
		return true;
	}

	void UndoHistory::Clear()
	{
		m_steps.clear();
		m_position = 0;
		m_isLastStepOpen = false;
	}
}
