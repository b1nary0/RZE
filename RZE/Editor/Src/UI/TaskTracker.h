#pragma once

#include <EngineCore/Async/AsyncOperation.h>

#include <chrono>
#include <deque>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace Editor
{
	// How the background tasks are doing overall, worst first when tasks are done
	enum class ETaskStatus
	{
		// Nothing has run yet, or the last task succeeded
		Ok,
		// A task is running
		Running,
		// The last task succeeded with warnings, or was cancelled
		Warning,
		// The last task failed
		Failed
	};

	enum class EHistoryKind
	{
		Started,
		// AsyncOperation::LogStep
		Step,
		// AsyncOperation::AddWarning
		Warning,
		Finished,
		FinishedWithWarnings,
		Failed,
		Cancelled
	};

	// Follows the operations the status bar shows: keeps a short note about the last one to finish until another does,
	// and a history of everything that happened to each (started, the steps it logged, its warnings, how it ended).
	// Which operations it's given is up to the manager's visibility filter (Settings > Background Tasks). Holds no UI, so
	// it can be tested.
	class TaskTracker
	{
	public:
		// The history keeps this many entries, dropping the oldest. Enough for a session: a folder of thumbnails is a step
		// per image.
		static constexpr size_t k_maxHistoryEntries = 20000;

		struct Note
		{
			// The task's name
			std::string Name;
			// One line, e.g. "<name> failed: <error>"
			std::string Text;
			// The task's whole warning, error or status text, which can run over several lines; empty if it had none
			std::string Details;
			// Succeeded, Failed or Cancelled
			EAsyncOperationState Result = EAsyncOperationState::Succeeded;
			// Only when it succeeded
			bool HasWarnings = false;
		};

		struct HistoryEntry
		{
			std::chrono::system_clock::time_point Time;
			EHistoryKind Kind;
			// The task's name
			std::string Task;
			// Without the task's name: "Started", the step or warning, or how it ended, e.g. "Failed: <error>"
			std::string Text;
			// Which run of a task it's from: a number given out as each task starts, so tasks running at once (even with
			// the same name) stay apart
			size_t Run = 0;
		};

		// One run of a task in the history, from when it started to how it ended
		struct HistoryRun
		{
			size_t Run = 0;
			// The task's name
			std::string Task;
			// Its entries, oldest first. They point into the history, so they're only good until it next changes.
			std::vector<const HistoryEntry*> Entries;

			// Whether its last entry says how it ended. One that hasn't is still running, or isn't shown any more.
			bool HasEnded() const;
			// Whether it failed, was cancelled or warned
			bool HasProblems() const;
		};

	public:
		// Once a frame, with AsyncOperationManager::GetVisibleOperations
		void Update(const std::vector<std::shared_ptr<AsyncOperation>>& visibleOperations);

		// Oldest first
		const std::vector<std::shared_ptr<AsyncOperation>>& GetRunning() const { return m_running; }
		// The note about the last task to finish, or null if none has
		const Note* GetLastNote() const { return m_hasNote ? &m_lastNote : nullptr; }
		ETaskStatus GetStatus() const;
		// Oldest first
		const std::deque<HistoryEntry>& GetHistory() const { return m_history; }
		// Running tasks carry on into the history afresh
		void ClearHistory();
		// The running operation a run of the history is from, or null once it isn't running or shown
		const AsyncOperation* FindRunning(size_t run) const;

		// "<name> finished" (with its status text, if it has any), "<name> finished with warnings: <first warning>",
		// "<name> failed: <error>" or "<name> cancelled"
		static Note MakeNote(const AsyncOperation& finished);

		// Whether an entry of this kind says how a task ended
		static bool IsEnd(EHistoryKind kind);
		// The history's runs, in the order they started
		static std::vector<HistoryRun> GroupByRun(const std::deque<HistoryEntry>& history);

	private:
		// Adds the steps operation has logged since last time to the history
		void ReadLoggedSteps(const AsyncOperation& operation);
		void AddToHistory(std::chrono::system_clock::time_point time, EHistoryKind kind, const std::string& task, std::string&& text, size_t run);

	private:
		std::vector<std::shared_ptr<AsyncOperation>> m_running;

		struct Following
		{
			// How many of its logged steps are in the history
			size_t StepsRead = 0;
			// Its entries' HistoryEntry::Run
			size_t Run = 0;
		};
		std::unordered_map<const AsyncOperation*, Following> m_following;

		Note m_lastNote;
		bool m_hasNote = false;

		std::deque<HistoryEntry> m_history;
		size_t m_nextRun = 1;
	};
}
