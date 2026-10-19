#include <UI/StatusBar.h>

#include <UI/Framework/EditorContext.h>
#include <UI/Framework/EditorUI.h>
#include <UI/Panels/TaskHistoryPanel.h>
#include <UI/TaskColours.h>
#include <UI/TaskTracker.h>

#include <RZE.h>

#include <EngineCore/Async/AsyncOperationManager.h>
#include <EngineCore/Timing/FramePacer.h>

#include <Utils/TimeUtils.h>

#include <ImGui/imgui_internal.h> // BeginViewportSideBar, RenderTextEllipsis

#include <algorithm>
#include <cstdio>
#include <ctime>
#include <string>

namespace
{
	constexpr char k_taskListID[] = "##TaskList";

	// Matches the main menu bar, so the two bars are the same height
	constexpr float k_verticalPadding = 5.0f;
	constexpr float k_edgePadding = 8.0f;
	constexpr float k_itemGap = 16.0f;

	constexpr float k_readyWidth = 120.0f;
	// Room for the widest it gets in practice, e.g. 16.7 ms, 240 FPS and 12.5 ms idle
	constexpr float k_frameStatsWidth = 260.0f;
	// The tasks box; also the width of the hover panel and the task list above it
	constexpr float k_tasksBoxWidth = 240.0f;
	// The circle left of the box that says how the tasks are doing, at a glance
	constexpr float k_statusCircleRadius = 4.5f;
	constexpr float k_statusCircleGap = 6.0f;
	constexpr float k_tasksWidth = k_statusCircleRadius * 2.0f + k_statusCircleGap + k_tasksBoxWidth;

	// The tasks box sits this far in from the bar's top and bottom, with this much room inside its frame
	constexpr float k_boxInset = 2.0f;
	constexpr float k_boxPadding = 6.0f;
	constexpr float k_boxRounding = 3.0f;
	constexpr ImU32 k_boxColour = IM_COL32(0, 0, 0, 70);
	// The box fills with this as its oldest task progresses, so the progress takes no room of its own
	constexpr float k_progressFillAlpha = 0.35f;
	// Between the box and the panels above it
	constexpr float k_panelGap = 2.0f;
	// A small gap between parts of the box's one line
	constexpr float k_partGap = 6.0f;
	// The dot at the start of each task's line when hovering
	constexpr float k_glanceDotRadius = 3.0f;

	constexpr char k_noTasksText[] = "No background tasks";
	// The history scrolls within this height
	constexpr float k_historyHeight = 260.0f;
	// The history's text wraps to the popup's width, which settles over its first frames; it's kept on the newest entry
	// until then
	constexpr int k_historyOpeningFrames = 3;
	// The history shows each run's last few steps; the Task History window has every one
	constexpr size_t k_historyStepsPerRun = 5;

	float GetTextTop(const ImVec2& min, const ImVec2& max)
	{
		return min.y + (max.y - min.y - ImGui::GetFontSize()) * 0.5f;
	}

	// One line of text from left to right at y, cut short with "..." if it doesn't fit
	void DrawTextLine(ImDrawList* drawList, float left, float right, float y, ImU32 colour, const std::string& text)
	{
		if (right <= left || text.empty())
		{
			return;
		}

		const float lineHeight = ImGui::GetFontSize();
		drawList->PushClipRect(ImVec2(left, y), ImVec2(right, y + lineHeight), true);
		ImGui::PushStyleColor(ImGuiCol_Text, colour);
		ImGui::RenderTextEllipsis(drawList, ImVec2(left, y), ImVec2(right, y + lineHeight), right, text.c_str(), nullptr, nullptr);
		ImGui::PopStyleColor();
		drawList->PopClipRect();
	}

	// Text that's cut short with "..." at width, as a widget in the current window
	void EllipsizedText(const std::string& text, float width, ImU32 colour)
	{
		const ImVec2 position = ImGui::GetCursorScreenPos();
		DrawTextLine(ImGui::GetWindowDrawList(), position.x, position.x + width, position.y, colour, text);
		ImGui::Dummy(ImVec2(width, ImGui::GetFontSize()));
	}

	// How a task went, in a word, for the box
	const char* GetResultWord(const Editor::TaskTracker::Note& note)
	{
		switch (note.Result)
		{
		case EAsyncOperationState::Succeeded:
			return note.HasWarnings ? "Warnings" : "Done";
		case EAsyncOperationState::Failed:
			return "Failed";
		default:
			return "Cancelled";
		}
	}

	std::string FormatPercent(float progress)
	{
		return std::to_string(static_cast<int>(progress * 100.0f)) + "%";
	}

	// A task's progress and status under its name, in the hover panel and the task list
	void DrawTaskDetails(const AsyncOperation& operation)
	{
		const float progress = operation.GetProgress();
		// The accent, like the box's own progress
		ImGui::PushStyleColor(ImGuiCol_PlotHistogram, Editor::EditorUI::k_accentColour);
		ImGui::ProgressBar(progress, ImVec2(-FLT_MIN, 0.0f), FormatPercent(progress).c_str());
		ImGui::PopStyleColor();

		const std::string status = operation.GetStatusText();
		if (!status.empty())
		{
			ImGui::PushTextWrapPos(0.0f);
			ImGui::TextDisabled("%s", status.c_str());
			ImGui::PopTextWrapPos();
		}
	}

	// The hover panel and the task list sit just above the box, exactly as wide as it, so they don't move about
	void PlacePanelAbove(const ImVec2& boxMin, const ImVec2& boxMax)
	{
		const float width = boxMax.x - boxMin.x;
		ImGui::SetNextWindowPos(ImVec2(boxMax.x, boxMin.y - k_panelGap), ImGuiCond_Always, ImVec2(1.0f, 1.0f));
		ImGui::SetNextWindowSizeConstraints(ImVec2(width, 0.0f), ImVec2(width, FLT_MAX));
	}
}

namespace Editor
{
	StatusBar::StatusBar()
	{
		AddItem(EStatusBarSide::Left, k_readyWidth, [](EditorContext&, const ImVec2& min, const ImVec2& max)
			{
				ImGui::GetWindowDrawList()->AddText(ImVec2(min.x, GetTextTop(min, max)), ImGui::GetColorU32(ImGuiCol_TextDisabled), "Ready");
			});

		AddItem(EStatusBarSide::Left, k_frameStatsWidth, [this](EditorContext&, const ImVec2& min, const ImVec2& max)
			{
				DrawFrameStats(min, max);
			});

		AddItem(EStatusBarSide::Right, k_tasksWidth, [this](EditorContext& context, const ImVec2& min, const ImVec2& max)
			{
				DrawTasks(context, min, max);
			});
	}

	void StatusBar::AddItem(EStatusBarSide side, float width, DrawItem draw)
	{
		m_items.push_back({ side, width, std::move(draw) });
	}

	void StatusBar::Draw(EditorContext& context)
	{
		ImGuiViewport* const viewport = ImGui::GetMainViewport();
		const float height = ImGui::GetFontSize() + k_verticalPadding * 2.0f;
		const ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoSavedSettings;

		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImGui::GetStyleColorVec4(ImGuiCol_MenuBarBg));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		const bool isOpen = ImGui::BeginViewportSideBar("##StatusBar", viewport, ImGuiDir_Down, height, flags);
		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor();

		if (isOpen)
		{
			const ImVec2 barMin = ImGui::GetWindowPos();
			const ImVec2 barMax(barMin.x + ImGui::GetWindowWidth(), barMin.y + height);
			ImGui::GetWindowDrawList()->AddLine(barMin, ImVec2(barMax.x, barMin.y), ImGui::GetColorU32(ImGuiCol_Border));

			float left = barMin.x + k_edgePadding;
			float right = barMax.x - k_edgePadding;
			for (size_t itemIndex = 0; itemIndex < m_items.size(); ++itemIndex)
			{
				const Item& item = m_items[itemIndex];

				ImVec2 min(left, barMin.y);
				if (item.Side == EStatusBarSide::Left)
				{
					left += item.Width + k_itemGap;
				}
				else
				{
					min.x = right - item.Width;
					right = min.x - k_itemGap;
				}
				const ImVec2 max(min.x + item.Width, barMax.y);

				ImGui::PushID(static_cast<int>(itemIndex));
				item.Draw(context, min, max);
				ImGui::PopID();
			}
		}
		// Like Begin, whether or not it's open
		ImGui::End();
	}

	void StatusBar::DrawFrameStats(const ImVec2& min, const ImVec2& max) const
	{
		const FramePacer::Stats& stats = RZE().GetFramePacer().GetStats();

		char text[96];
		snprintf(text, sizeof(text), "%.1f ms \xC2\xB7 %.0f FPS \xC2\xB7 idle %.1f ms", stats.FrameTimeMS, stats.FramesPerSecond, stats.IdleTimeMS);
		ImGui::GetWindowDrawList()->AddText(ImVec2(min.x, GetTextTop(min, max)), ImGui::GetColorU32(ImGuiCol_TextDisabled), text);

		ImGui::SetCursorScreenPos(min);
		ImGui::InvisibleButton("##FrameStats", ImVec2(max.x - min.x, max.y - min.y));
		if (ImGui::IsItemHovered())
		{
			// Just above the bar, lined up with the stats, like the tasks' hover panel rather than following the mouse
			ImGui::SetNextWindowPos(ImVec2(min.x, min.y - k_panelGap), ImGuiCond_Always, ImVec2(0.0f, 1.0f));
			ImGui::BeginTooltip();
			ImGui::Text("%.2f ms a frame, %.1f frames a second", stats.FrameTimeMS, stats.FramesPerSecond);
			ImGui::Text("The UI updates %.1f times a second", stats.UIFramesPerSecond);
			ImGui::Text("%.2f ms of each frame idle:", stats.IdleTimeMS);
			ImGui::BulletText("%.2f ms on tasks", stats.IdleWorkMS);
			ImGui::BulletText("%.2f ms waiting for the next frame", std::max(0.0, stats.IdleTimeMS - stats.IdleWorkMS));
			ImGui::TextDisabled("Averaged over %.1f s. Frames only have idle time with vsync or a frame rate cap.", FramePacer::k_statsWindow);
			ImGui::EndTooltip();
		}
	}

	void StatusBar::DrawTasks(EditorContext& context, const ImVec2& min, const ImVec2& max)
	{
		const TaskTracker& tasks = context.GetTasks();
		ImDrawList* const drawList = ImGui::GetWindowDrawList();

		// The circle, then a box of its own, so it reads as one control whether or not anything's running. Both answer
		// the mouse.
		const ImVec2 boxMin(max.x - k_tasksBoxWidth, min.y + k_boxInset);
		const ImVec2 boxMax(max.x, max.y - k_boxInset);
		ImGui::SetCursorScreenPos(ImVec2(min.x, boxMin.y));
		ImGui::InvisibleButton("##Tasks", ImVec2(boxMax.x - min.x, boxMax.y - boxMin.y));
		const bool isHovered = ImGui::IsItemHovered();
		if (ImGui::IsItemClicked())
		{
			ImGui::OpenPopup(k_taskListID);
			// Opens on the newest
			m_historyOpeningFrames = k_historyOpeningFrames;
		}

		const std::vector<std::shared_ptr<AsyncOperation>>& running = tasks.GetRunning();

		const ImVec2 circleCentre(min.x + k_statusCircleRadius, (boxMin.y + boxMax.y) * 0.5f);
		drawList->AddCircleFilled(circleCentre, k_statusCircleRadius, ImGui::GetColorU32(TaskColours::ForStatus(tasks.GetStatus())));

		drawList->AddRectFilled(boxMin, boxMax, isHovered ? ImGui::GetColorU32(ImGuiCol_HeaderHovered) : k_boxColour, k_boxRounding);
		if (!running.empty())
		{
			const float progress = std::clamp(running.front()->GetProgress(), 0.0f, 1.0f);
			if (progress > 0.0f)
			{
				const ImVec2 fillMax(boxMin.x + (boxMax.x - boxMin.x) * progress, boxMax.y);
				drawList->AddRectFilled(boxMin, fillMax, EditorUI::WithAlpha(EditorUI::k_accentColour, k_progressFillAlpha), k_boxRounding);
			}
		}
		drawList->AddRect(boxMin, boxMax, ImGui::GetColorU32(ImGuiCol_Border), k_boxRounding);

		float left = boxMin.x + k_boxPadding;
		float right = boxMax.x - k_boxPadding;
		const float textTop = GetTextTop(min, max);

		if (!running.empty())
		{
			// What matters most is kept whole, from the right: how many others are running, then how far the oldest
			// has got. Its name takes what's left; its status is in the hover panel.
			if (running.size() > 1)
			{
				const std::string others = "+" + std::to_string(running.size() - 1);
				const float othersWidth = ImGui::CalcTextSize(others.c_str()).x;
				DrawTextLine(drawList, right - othersWidth, right, textTop, ImGui::GetColorU32(ImGuiCol_TextDisabled), others);
				right -= othersWidth + k_partGap;
			}

			const std::string percent = FormatPercent(running.front()->GetProgress());
			const float percentWidth = ImGui::CalcTextSize(percent.c_str()).x;
			DrawTextLine(drawList, right - percentWidth, right, textTop, ImGui::GetColorU32(ImGuiCol_Text), percent);
			right -= percentWidth + k_partGap;

			DrawTextLine(drawList, left, right, textTop, ImGui::GetColorU32(ImGuiCol_Text), running.front()->GetName());
		}
		else if (const TaskTracker::Note* const note = tasks.GetLastNote())
		{
			// Laid out like a running task: how it went is kept whole at the right, in its colour, where the
			// percentage was. The rest of the result is in the hover panel.
			const char* const result = GetResultWord(*note);
			const float resultWidth = ImGui::CalcTextSize(result).x;
			DrawTextLine(drawList, right - resultWidth, right, textTop, ImGui::GetColorU32(TaskColours::ForStatus(tasks.GetStatus())), result);
			right -= resultWidth + k_partGap;

			DrawTextLine(drawList, left, right, textTop, ImGui::GetColorU32(ImGuiCol_TextDisabled), note->Name);
		}
		else
		{
			DrawTextLine(drawList, left, right, textTop, ImGui::GetColorU32(ImGuiCol_TextDisabled), k_noTasksText);
		}

		if (isHovered && !ImGui::IsPopupOpen(k_taskListID))
		{
			PlacePanelAbove(boxMin, boxMax);
			ImGui::BeginTooltip();
			if (running.empty())
			{
				DrawLastResult(tasks);
				ImGui::TextDisabled("Click for the history");
			}
			else
			{
				DrawTasksAtAGlance(tasks);
				ImGui::TextDisabled("Click to cancel tasks");
			}
			ImGui::EndTooltip();
		}

		PlacePanelAbove(boxMin, boxMax);
		if (ImGui::BeginPopup(k_taskListID))
		{
			if (ImGui::IsKeyPressed(ImGuiKey_Escape, false))
			{
				ImGui::CloseCurrentPopup();
			}
			if (running.empty())
			{
				DrawHistory(context);
			}
			else
			{
				DrawRunningTasks(tasks);
			}
			ImGui::EndPopup();
		}
	}

	void StatusBar::DrawTasksAtAGlance(const TaskTracker& tasks) const
	{
		const std::vector<std::shared_ptr<AsyncOperation>>& running = tasks.GetRunning();
		ImGui::TextDisabled("%zu %s running", running.size(), running.size() == 1 ? "task" : "tasks");

		// A line each: a dot for how it's going, its name, and on the right how long it has left and how far it's got
		ImDrawList* const drawList = ImGui::GetWindowDrawList();
		const float lineHeight = ImGui::GetTextLineHeightWithSpacing();
		const float width = ImGui::GetContentRegionAvail().x;
		for (const std::shared_ptr<AsyncOperation>& operation : running)
		{
			const ImVec2 lineMin = ImGui::GetCursorScreenPos();
			const float textTop = lineMin.y;
			float left = lineMin.x;
			float right = lineMin.x + width;

			ImVec4 dotColour = TaskColours::k_running;
			if (operation->IsCancellationRequested())
			{
				dotColour = ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled);
			}
			else if (operation->HasWarnings())
			{
				dotColour = TaskColours::k_warning;
			}
			drawList->AddCircleFilled(ImVec2(left + k_glanceDotRadius, textTop + ImGui::GetFontSize() * 0.5f), k_glanceDotRadius, ImGui::GetColorU32(dotColour));
			left += k_glanceDotRadius * 2.0f + k_partGap;

			if (operation->IsCancellationRequested())
			{
				const char* const cancelling = "Cancelling";
				const float cancellingWidth = ImGui::CalcTextSize(cancelling).x;
				DrawTextLine(drawList, right - cancellingWidth, right, textTop, ImGui::GetColorU32(ImGuiCol_TextDisabled), cancelling);
				right -= cancellingWidth + k_partGap;
			}
			else
			{
				const float progress = operation->GetProgress();
				const std::string percent = FormatPercent(progress);
				const float percentWidth = ImGui::CalcTextSize(percent.c_str()).x;
				DrawTextLine(drawList, right - percentWidth, right, textTop, ImGui::GetColorU32(ImGuiCol_Text), percent);
				right -= percentWidth + k_partGap;

				const std::optional<double> secondsLeft = TaskTracker::EstimateSecondsLeft(tasks.GetSecondsRunning(*operation), progress);
				if (secondsLeft.has_value())
				{
					const std::string timeLeft = "~" + TaskTracker::FormatDuration(*secondsLeft);
					const float timeLeftWidth = ImGui::CalcTextSize(timeLeft.c_str()).x;
					DrawTextLine(drawList, right - timeLeftWidth, right, textTop, ImGui::GetColorU32(ImGuiCol_TextDisabled), timeLeft);
					right -= timeLeftWidth + k_partGap;
				}
			}

			DrawTextLine(drawList, left, right, textTop, ImGui::GetColorU32(ImGuiCol_Text), operation->GetName());
			ImGui::Dummy(ImVec2(width, lineHeight));
		}
	}

	void StatusBar::DrawRunningTasks(const TaskTracker& tasks)
	{
		for (const std::shared_ptr<AsyncOperation>& operation : tasks.GetRunning())
		{
			ImGui::PushID(operation.get());

			// The name takes the line, less room for cancelling at the right
			const char* const cancelText = operation->IsCancellationRequested() ? "Cancelling" : "x";
			const float cancelWidth = operation->IsCancellationRequested()
				? ImGui::CalcTextSize(cancelText).x
				: ImGui::CalcTextSize(cancelText).x + ImGui::GetStyle().FramePadding.x * 2.0f;
			const float nameWidth = ImGui::GetContentRegionAvail().x - cancelWidth - k_partGap;

			EllipsizedText(operation->GetName(), nameWidth, ImGui::GetColorU32(ImGuiCol_Text));
			ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - cancelWidth);
			if (operation->IsCancellationRequested())
			{
				ImGui::TextDisabled("%s", cancelText);
			}
			else
			{
				if (ImGui::SmallButton(cancelText))
				{
					operation->RequestCancel();
				}
				EditorUI::ItemTooltip("Cancel");
			}

			DrawTaskDetails(*operation);
			ImGui::Spacing();

			ImGui::PopID();
		}
	}

	void StatusBar::DrawLastResult(const TaskTracker& tasks) const
	{
		ImGui::TextDisabled("%s", k_noTasksText);
		const TaskTracker::Note* const note = tasks.GetLastNote();
		if (note == nullptr)
		{
			return;
		}

		ImGui::Spacing();
		ImGui::TextDisabled("Last task");
		ImGui::PushTextWrapPos(0.0f);
		const ETaskStatus status = tasks.GetStatus();
		ImGui::TextColored(status == ETaskStatus::Ok ? ImGui::GetStyleColorVec4(ImGuiCol_Text) : TaskColours::ForStatus(status), "%s", note->Text.c_str());
		// Only when there's more than the line above already shows
		if (note->Details.find('\n') != std::string::npos)
		{
			ImGui::TextDisabled("%s", note->Details.c_str());
		}
		ImGui::PopTextWrapPos();
	}

	void StatusBar::DrawHistory(EditorContext& context)
	{
		ImGui::TextDisabled("History");
		const char* const fullHistory = "Full history";
		ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - ImGui::CalcTextSize(fullHistory).x - ImGui::GetStyle().FramePadding.x * 2.0f);
		if (ImGui::SmallButton(fullHistory))
		{
			if (TaskHistoryPanel* const panel = context.GetPanels().Find<TaskHistoryPanel>())
			{
				panel->Show();
			}
			ImGui::CloseCurrentPopup();
		}
		EditorUI::ItemTooltip("Every task and everything it did, in a window of its own");

		const TaskTracker& tasks = context.GetTasks();
		if (tasks.GetHistory().empty())
		{
			ImGui::TextDisabled("Nothing has run yet");
			return;
		}

		ImGui::BeginChild("##History", ImVec2(0.0f, k_historyHeight));
		// As of last frame, before anything new is added below
		const bool wasAtNewest = ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 1.0f;

		ImGui::PushTextWrapPos(0.0f);
		const std::vector<TaskTracker::HistoryRun> runs = TaskTracker::GroupByRun(tasks.GetHistory());
		for (size_t runIndex = 0; runIndex < runs.size(); ++runIndex)
		{
			const TaskTracker::HistoryRun& run = runs[runIndex];
			if (runIndex > 0)
			{
				ImGui::Spacing();
			}
			ImGui::TextColored(EditorUI::k_headingColour, "%s", run.Task.c_str());

			// Only its last few steps: the earlier ones are counted on a line where they'd be
			const size_t stepCount = static_cast<size_t>(std::count_if(run.Entries.begin(), run.Entries.end(),
				[](const TaskTracker::HistoryEntry* entry) { return entry->Kind == EHistoryKind::Step; }));
			const size_t skippedCount = stepCount > k_historyStepsPerRun ? stepCount - k_historyStepsPerRun : 0;
			size_t stepsSeen = 0;
			for (const TaskTracker::HistoryEntry* const entry : run.Entries)
			{
				if (entry->Kind == EHistoryKind::Step && stepsSeen++ < skippedCount)
				{
					if (stepsSeen == 1)
					{
						ImGui::TextDisabled("...and %zu more in the full history", skippedCount);
					}
					continue;
				}

				ImGui::TextDisabled("%s", TimeUtils::FormatClockTime(entry->Time).c_str());
				ImGui::SameLine();
				ImGui::TextColored(TaskColours::ForHistory(entry->Kind), "%s", entry->Text.c_str());
			}
		}
		ImGui::PopTextWrapPos();

		if (m_historyOpeningFrames > 0 || wasAtNewest)
		{
			ImGui::SetScrollHereY(1.0f);
		}
		m_historyOpeningFrames = std::max(0, m_historyOpeningFrames - 1);
		ImGui::EndChild();
	}
}
