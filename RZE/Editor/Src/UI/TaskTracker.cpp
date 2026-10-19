#include <UI/TaskTracker.h>

#include <EngineCore/Async/AsyncOperation.h>

#include <algorithm>
#include <cmath>

namespace
{
	// Status and error text can run over several lines; a note has room for one
	std::string FirstLine(const std::string& text)
	{
		return text.substr(0, text.find('\n'));
	}

	// How it ended, without the task's name
	std::string GetEndText(const Editor::TaskTracker::Note& note)
	{
		switch (note.Result)
		{
		case EAsyncOperationState::Failed:
			return "Failed: " + FirstLine(note.Details);
		case EAsyncOperationState::Cancelled:
			return "Cancelled";
		default:
		{
			const std::string details = FirstLine(note.Details);
			if (note.HasWarnings)
			{
				return "Finished with warnings: " + details;
			}
			return details.empty() ? "Finished" : "Finished: " + details;
		}
		}
	}

	Editor::EHistoryKind GetEndKind(const Editor::TaskTracker::Note& note)
	{
		switch (note.Result)
		{
		case EAsyncOperationState::Failed:
			return Editor::EHistoryKind::Failed;
		case EAsyncOperationState::Cancelled:
			return Editor::EHistoryKind::Cancelled;
		default:
			return note.HasWarnings ? Editor::EHistoryKind::FinishedWithWarnings : Editor::EHistoryKind::Finished;
		}
	}
}

namespace Editor
{
	void TaskTracker::Update(const std::vector<std::shared_ptr<AsyncOperation>>& visibleOperations)
	{
		// Those new since last frame have started
		for (const std::shared_ptr<AsyncOperation>& operation : visibleOperations)
		{
			if (m_following.find(operation.get()) == m_following.end())
			{
				m_following[operation.get()] = { std::chrono::steady_clock::now(), 0 };
				AddToHistory(std::chrono::system_clock::now(), EHistoryKind::Started, operation->GetName(), "Started");
			}
			ReadLoggedSteps(*operation);
		}

		// Those gone from the list since last frame have finished, or aren't shown any more
		for (const std::shared_ptr<AsyncOperation>& operation : m_running)
		{
			const bool isStillRunning = std::find(visibleOperations.begin(), visibleOperations.end(), operation) != visibleOperations.end();
			if (isStillRunning)
			{
				continue;
			}

			if (operation->IsFinished())
			{
				// Whatever it logged on its last frame comes before how it ended
				ReadLoggedSteps(*operation);

				m_lastNote = MakeNote(*operation);
				m_hasNote = true;
				AddToHistory(std::chrono::system_clock::now(), GetEndKind(m_lastNote), m_lastNote.Name, GetEndText(m_lastNote));
			}
			m_following.erase(operation.get());
		}

		m_running = visibleOperations;
	}

	void TaskTracker::ReadLoggedSteps(const AsyncOperation& operation)
	{
		size_t& stepsRead = m_following[&operation].StepsRead;
		for (const AsyncOperation::LoggedStep& step : operation.GetLoggedSteps(stepsRead))
		{
			AddToHistory(step.Time, step.IsWarning ? EHistoryKind::Warning : EHistoryKind::Step, operation.GetName(), step.Text);
			++stepsRead;
		}
	}

	void TaskTracker::AddToHistory(std::chrono::system_clock::time_point time, EHistoryKind kind, const std::string& task, std::string text)
	{
		m_history.push_back({ time, kind, task, std::move(text) });
		while (m_history.size() > k_maxHistoryEntries)
		{
			m_history.pop_front();
		}
	}

	double TaskTracker::GetSecondsRunning(const AsyncOperation& operation) const
	{
		const auto following = m_following.find(&operation);
		if (following == m_following.end())
		{
			return 0.0;
		}
		return std::chrono::duration<double>(std::chrono::steady_clock::now() - following->second.Started).count();
	}

	std::optional<double> TaskTracker::EstimateSecondsLeft(double secondsRunning, float progress)
	{
		if (progress < k_minProgressToEstimate || secondsRunning < 1.0)
		{
			return std::nullopt;
		}
		if (progress >= 1.0f)
		{
			return 0.0;
		}
		// At the rate it's gone so far
		return secondsRunning * (1.0 - progress) / progress;
	}

	std::string TaskTracker::FormatDuration(double seconds)
	{
		const long long total = std::max(0LL, static_cast<long long>(std::llround(seconds)));
		char text[32];
		if (total < 60)
		{
			snprintf(text, sizeof(text), "%llds", total);
		}
		else if (total < 3600)
		{
			snprintf(text, sizeof(text), "%lldm %02llds", total / 60, total % 60);
		}
		else
		{
			snprintf(text, sizeof(text), "%lldh %02lldm", total / 3600, (total % 3600) / 60);
		}
		return text;
	}

	ETaskStatus TaskTracker::GetStatus() const
	{
		if (!m_running.empty())
		{
			return ETaskStatus::Running;
		}
		if (!m_hasNote)
		{
			return ETaskStatus::Ok;
		}

		switch (m_lastNote.Result)
		{
		case EAsyncOperationState::Failed:
			return ETaskStatus::Failed;
		case EAsyncOperationState::Cancelled:
			return ETaskStatus::Warning;
		default:
			return m_lastNote.HasWarnings ? ETaskStatus::Warning : ETaskStatus::Ok;
		}
	}

	TaskTracker::Note TaskTracker::MakeNote(const AsyncOperation& finished)
	{
		Note note;
		note.Name = finished.GetName();
		note.Result = finished.GetState();
		switch (note.Result)
		{
		case EAsyncOperationState::Failed:
			note.Text = finished.GetName() + " failed: " + FirstLine(finished.GetErrorText());
			note.Details = finished.GetErrorText();
			break;

		case EAsyncOperationState::Cancelled:
			note.Text = finished.GetName() + " cancelled";
			break;

		default:
		{
			if (finished.HasWarnings())
			{
				note.Details = finished.GetWarningText();
				note.Text = finished.GetName() + " finished with warnings: " + FirstLine(note.Details);
				note.HasWarnings = true;
				break;
			}

			note.Details = finished.GetStatusText();
			const std::string status = FirstLine(note.Details);
			note.Text = finished.GetName() + " finished" + (status.empty() ? "" : ": " + status);
			break;
		}
		}
		return note;
	}
}
