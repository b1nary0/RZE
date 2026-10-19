#pragma once

#include <ImGui/imgui.h>

#include <functional>
#include <vector>

namespace Editor
{
	class EditorContext;
	class TaskTracker;

	enum class EStatusBarSide
	{
		Left,
		Right
	};

	// The bar along the bottom of the editor. It takes its height out of the viewport's work area, so the dock space
	// fits above it. It's made of items on either side, each a fixed width: left items go from the left edge and right
	// items from the right edge, both in the order they're added. It starts with "Ready" and the frame stats on the
	// left, and the running tasks on the right.
	//
	// The frame stats are the frame time, frames per second and idle time per frame, averaged over half a second
	// (FramePacer::Stats). Hovering splits the idle time into what went to tasks and what was spent waiting.
	//
	// The tasks area starts with a circle that says how the tasks are doing at a glance (ETaskStatus): green when the
	// last one succeeded or none has run, yellow while any is running, orange when the last had warnings or was
	// cancelled, and red when it failed.
	//
	// Then comes a small box of its own that's always there, filling with the accent colour as the oldest running task
	// progresses. Which tasks it shows is up to Settings > Background Tasks. On its one line it keeps whole how many
	// others are running ("+2") and the oldest's percentage, and fits its name in what's left. With none running, it
	// has the last one's name with how it went in that place instead, in the circle's colour ("Done", "Warnings",
	// "Cancelled" or "Failed"), or "No background tasks" grayed out if there hasn't been one.
	//
	// Hovering shows a line for each running task at a glance (a dot for how it's going, its name, roughly how long it
	// has left, and its percentage), or how the last one went. Clicking opens a list with each running task's progress
	// and status and a button to cancel it, or with none running, the history: when each task started and ended, and
	// every step and warning it logged. Both sit just above the box, as wide as it.
	class StatusBar
	{
	public:
		// Draws into the rectangle from min to max. A widget needs ImGui::SetCursorScreenPos(min) first.
		using DrawItem = std::function<void(EditorContext& context, const ImVec2& min, const ImVec2& max)>;

	public:
		StatusBar();

	public:
		void Draw(EditorContext& context);

		void AddItem(EStatusBarSide side, float width, DrawItem draw);

	private:
		void DrawFrameStats(const ImVec2& min, const ImVec2& max) const;
		void DrawTasks(EditorContext& context, const ImVec2& min, const ImVec2& max);
		// On hover: a line for each running task, with a dot for how it's going, its name, roughly how long it has left
		// and how far it's got
		void DrawTasksAtAGlance(const TaskTracker& tasks) const;
		// On click: each running task's progress and status, and a button to cancel it
		void DrawRunningTasks(const TaskTracker& tasks);
		// On hover with none running: how the last task went, in full
		void DrawLastResult(const TaskTracker& tasks) const;
		// Everything that happened to every task, oldest first, under a heading for each run of a task, with only its last
		// few steps; the Task History window has the rest. It opens on the newest and follows new ones while it's scrolled
		// to the bottom.
		void DrawHistory(EditorContext& context);

	private:
		struct Item
		{
			EStatusBarSide Side;
			float Width;
			DrawItem Draw;
		};

		std::vector<Item> m_items;
		// Frames left to keep the history on its newest entry after it opens, while its wrapped lines settle
		int m_historyOpeningFrames = 0;
	};
}
