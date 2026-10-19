#pragma once

#include <UI/Framework/EditorPanel.h>
#include <UI/TaskTracker.h>

#include <string>
#include <vector>

namespace Editor
{
	// Every background task's history in full (TaskTracker): a row for each run of a task, newest first, with how it
	// ended, when it started and how long it took, that opens to list everything it did, e.g. each image a "Loading
	// thumbnails" loaded. It searches task names and what they did, can show only the runs that went wrong, and copies or
	// clears the history. The status bar's history opens it ("Full history").
	class TaskHistoryPanel : public EditorPanel
	{
	public:
		static PanelDesc Describe();

		// Opens it, in front
		void Show();

	private:
		void Draw(EditorContext& context) override;

		void DrawToolbar(TaskTracker& tasks, const std::vector<const TaskTracker::HistoryRun*>& shownRuns);
		void DrawRun(const TaskTracker& tasks, const TaskTracker::HistoryRun& run);

		// Whether the run is shown: it went wrong, if only those are, and its task or something it did matches the search
		bool IsShown(const TaskTracker::HistoryRun& run) const;
		// Whether the entry is listed under its open run: everything, unless the search matches only some of what it did
		bool IsListed(const TaskTracker::HistoryRun& run, const TaskTracker::HistoryEntry& entry) const;

		// The runs shown, oldest first, as text, e.g. for the clipboard
		static std::string FormatAsText(const std::vector<const TaskTracker::HistoryRun*>& runs);

	private:
		char m_search[128] = {};
		bool m_isProblemsOnly = false;
		// Show() was called; it's brought to the front the next time it draws
		bool m_isFocusPending = false;
	};
}
