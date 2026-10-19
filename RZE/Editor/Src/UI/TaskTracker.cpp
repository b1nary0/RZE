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
				const size_t run = m_nextRun++;
				m_following[operation.get()] = { std::chrono::steady_clock::now(), 0, run };
				AddToHistory(std::chrono::system_clock::now(), EHistoryKind::Started, operation->GetName(), "Started", run);
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
				AddToHistory(std::chrono::system_clock::now(), GetEndKind(m_lastNote), m_lastNote.Name, GetEndText(m_lastNote), m_following[operation.get()].Run);
			}
			m_following.erase(operation.get());
		}

		m_running = visibleOperations;
	}

	void TaskTracker::ReadLoggedSteps(const AsyncOperation& operation)
	{
		Following& following = m_following[&operation];
		for (AsyncOperation::LoggedStep& step : operation.GetLoggedSteps(following.StepsRead))
		{
			AddToHistory(step.Time, step.IsWarning ? EHistoryKind::Warning : EHistoryKind::Step, operation.GetName(), std::move(step.Text), following.Run);
			++following.StepsRead;
		}
	}

	void TaskTracker::AddToHistory(std::chrono::system_clock::time_point time, EHistoryKind kind, const std::string& task, std::string&& text, size_t run)
	{
		m_history.push_back({ time, kind, task, std::move(text), run });
		while (m_history.size() > k_maxHistoryEntries)
		{
			m_history.pop_front();
		}
	}

	void TaskTracker::ClearHistory()
	{
		m_history.clear();
	}

	const AsyncOperation* TaskTracker::FindRunning(size_t run) const
	{
		for (const std::shared_ptr<AsyncOperation>& operation : m_running)
		{
			const auto following = m_following.find(operation.get());
			if (following != m_following.end() && following->second.Run == run)
			{
				return operation.get();
			}
		}
		return nullptr;
	}

	bool TaskTracker::IsEnd(EHistoryKind kind)
	{
		return kind == EHistoryKind::Finished || kind == EHistoryKind::FinishedWithWarnings || kind == EHistoryKind::Failed
			|| kind == EHistoryKind::Cancelled;
	}

	std::vector<TaskTracker::HistoryRun> TaskTracker::GroupByRun(const std::deque<HistoryEntry>& history)
	{
		std::vector<HistoryRun> runs;
		// Each run's place in runs
		std::unordered_map<size_t, size_t> runIndices;
		for (const HistoryEntry& entry : history)
		{
			const auto found = runIndices.find(entry.Run);
			if (found == runIndices.end())
			{
				runIndices.emplace(entry.Run, runs.size());
				HistoryRun& run = runs.emplace_back();
				run.Run = entry.Run;
				run.Task = entry.Task;
				run.Entries.push_back(&entry);
			}
			else
			{
				runs[found->second].Entries.push_back(&entry);
			}
		}
		return runs;
	}

	bool TaskTracker::HistoryRun::HasEnded() const
	{
		return !Entries.empty() && IsEnd(Entries.back()->Kind);
	}

	bool TaskTracker::HistoryRun::HasProblems() const
	{
		return std::any_of(Entries.begin(), Entries.end(), [](const HistoryEntry* entry)
			{
				return entry->Kind == EHistoryKind::Warning || entry->Kind == EHistoryKind::FinishedWithWarnings
					|| entry->Kind == EHistoryKind::Failed || entry->Kind == EHistoryKind::Cancelled;
			});
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
