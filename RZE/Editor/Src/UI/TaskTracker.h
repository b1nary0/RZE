#pragma once

#include <EngineCore/Async/AsyncOperation.h>

#include <chrono>
#include <deque>
#include <memory>
#include <optional>
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
	// Blocking operations are left out, since the progress modal shows those. Holds no UI, so it can be tested.
	class TaskTracker
	{
	public:
		// The history keeps this many entries, dropping the oldest
		static constexpr size_t k_maxHistoryEntries = 1000;

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
		// How long a running task has been running for, from when Update first saw it; 0 for one it isn't following
		double GetSecondsRunning(const AsyncOperation& operation) const;

		// "<name> finished" (with its status text, if it has any), "<name> finished with warnings: <first warning>",
		// "<name> failed: <error>" or "<name> cancelled"
		static Note MakeNote(const AsyncOperation& finished);

		// How long a task that's this far through after this long has left, if it's gone on long enough to tell: at
		// least k_minProgressToEstimate done, and a second
		static std::optional<double> EstimateSecondsLeft(double secondsRunning, float progress);
		static constexpr float k_minProgressToEstimate = 0.05f;
		// Short, e.g. "8s", "1m 05s", "2h 03m"
		static std::string FormatDuration(double seconds);

	private:
		// Adds the steps operation has logged since last time to the history
		void ReadLoggedSteps(const AsyncOperation& operation);
		void AddToHistory(std::chrono::system_clock::time_point time, EHistoryKind kind, const std::string& task, std::string text);

	private:
		std::vector<std::shared_ptr<AsyncOperation>> m_running;

		struct Following
		{
			std::chrono::steady_clock::time_point Started;
			// How many of its logged steps are in the history
			size_t StepsRead = 0;
		};
		std::unordered_map<const AsyncOperation*, Following> m_following;

		Note m_lastNote;
		bool m_hasNote = false;

		std::deque<HistoryEntry> m_history;
	};
}
