#include <UI/Panels/TaskHistoryPanel.h>

#include <UI/Framework/EditorContext.h>
#include <UI/Framework/EditorUI.h>
#include <UI/Framework/PanelRegistry.h>
#include <UI/TaskColours.h>

#include <EngineCore/Async/AsyncOperation.h>

#include <Utils/StringUtils.h>
#include <Utils/TimeUtils.h>

#include <ImGui/imgui.h>

#include <algorithm>
#include <chrono>
#include <cstdio>

namespace
{
	// Font sizes across, for the columns that don't stretch
	constexpr float k_startedColumnWidth = 5.0f;
	constexpr float k_tookColumnWidth = 4.5f;
	constexpr float k_searchWidth = 18.0f;

	struct RunResult
	{
		std::string Text;
		ImVec4 Colour;
	};

	// How the run ended, or how far it's got while it's still running
	RunResult GetRunResult(const Editor::TaskTracker& tasks, const Editor::TaskTracker::HistoryRun& run)
	{
		if (const AsyncOperation* const running = tasks.FindRunning(run.Run))
		{
			char text[32];
			snprintf(text, sizeof(text), "Running, %d%%", static_cast<int>(running->GetProgress() * 100.0f));
			return { text, Editor::TaskColours::k_running };
		}
		if (run.HasEnded())
		{
			const Editor::TaskTracker::HistoryEntry& end = *run.Entries.back();
			return { end.Text, Editor::TaskColours::ForHistory(end.Kind) };
		}
		// Hidden by Settings > Background Tasks before it ended
		return { "Not shown any more", ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled) };
	}

	// How long it's been running, while it is; otherwise from its first entry to its last
	double GetSecondsTaken(const Editor::TaskTracker& tasks, const Editor::TaskTracker::HistoryRun& run)
	{
		if (const AsyncOperation* const running = tasks.FindRunning(run.Run))
		{
			return running->GetSecondsRunning();
		}
		return std::chrono::duration<double>(run.Entries.back()->Time - run.Entries.front()->Time).count();
	}
}

namespace Editor
{
	REGISTER_EDITOR_PANEL(TaskHistoryPanel);

	PanelDesc TaskHistoryPanel::Describe()
	{
		PanelDesc desc;
		desc.Name = "Task History";
		desc.MenuPath = "View/Windows/Task History";
		desc.DefaultDock = EPanelDock::Floating;
		desc.DefaultSize = ImVec2(820.0f, 480.0f);
		desc.DefaultOpen = false;
		desc.WindowFlags = ImGuiWindowFlags_NoCollapse;
		return desc;
	}

	void TaskHistoryPanel::Show()
	{
		SetOpen(true);
		m_isFocusPending = true;
	}

	void TaskHistoryPanel::Draw(EditorContext& context)
	{
		if (m_isFocusPending)
		{
			ImGui::SetWindowFocus();
			m_isFocusPending = false;
		}

		TaskTracker& tasks = context.GetTasks();
		const std::vector<TaskTracker::HistoryRun> runs = TaskTracker::GroupByRun(tasks.GetHistory());
		std::vector<const TaskTracker::HistoryRun*> shownRuns;
		for (const TaskTracker::HistoryRun& run : runs)
		{
			if (IsShown(run))
			{
				shownRuns.push_back(&run);
			}
		}

		DrawToolbar(tasks, shownRuns);

		if (shownRuns.empty())
		{
			ImGui::TextDisabled("%s", runs.empty() ? "Nothing has run yet" : "Nothing matches");
			return;
		}

		const ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable
			| ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchProp;
		if (!ImGui::BeginTable("##Runs", 4, flags, ImGui::GetContentRegionAvail()))
		{
			return;
		}

		const float fontSize = ImGui::GetFontSize();
		ImGui::TableSetupColumn("Task", ImGuiTableColumnFlags_WidthStretch, 3.0f);
		ImGui::TableSetupColumn("Result", ImGuiTableColumnFlags_WidthStretch, 2.0f);
		ImGui::TableSetupColumn("Started", ImGuiTableColumnFlags_WidthFixed, fontSize * k_startedColumnWidth);
		ImGui::TableSetupColumn("Took", ImGuiTableColumnFlags_WidthFixed, fontSize * k_tookColumnWidth);
		ImGui::TableSetupScrollFreeze(0, 1);
		ImGui::TableHeadersRow();

		// Newest first
		for (auto run = shownRuns.rbegin(); run != shownRuns.rend(); ++run)
		{
			DrawRun(tasks, **run);
		}

		ImGui::EndTable();
	}

	void TaskHistoryPanel::DrawToolbar(TaskTracker& tasks, const std::vector<const TaskTracker::HistoryRun*>& shownRuns)
	{
		ImGui::SetNextItemWidth(ImGui::GetFontSize() * k_searchWidth);
		ImGui::InputTextWithHint("##search", "Search tasks and what they did", m_search, sizeof(m_search));

		ImGui::SameLine();
		ImGui::Checkbox("Problems only", &m_isProblemsOnly);
		EditorUI::ItemTooltip("Only the runs that failed, were cancelled or warned");

		ImGui::SameLine();
		ImGui::BeginDisabled(shownRuns.empty());
		if (ImGui::Button("Copy"))
		{
			ImGui::SetClipboardText(FormatAsText(shownRuns).c_str());
		}
		EditorUI::ItemTooltip("Copies the runs shown, and everything they did, as text");
		ImGui::EndDisabled();

		ImGui::SameLine();
		ImGui::BeginDisabled(tasks.GetHistory().empty());
		if (ImGui::Button("Clear"))
		{
			tasks.ClearHistory();
		}
		EditorUI::ItemTooltip("Forgets the history; tasks still running carry on into it");
		ImGui::EndDisabled();

		ImGui::SameLine();
		ImGui::TextDisabled("%zu %s", shownRuns.size(), shownRuns.size() == 1 ? "run" : "runs");
	}

	void TaskHistoryPanel::DrawRun(const TaskTracker& tasks, const TaskTracker::HistoryRun& run)
	{
		ImGui::PushID(static_cast<int>(run.Run));
		ImGui::TableNextRow();

		ImGui::TableSetColumnIndex(0);
		// Open while the search matches only some of what it did, so those show
		const bool isSearchingWithin = m_search[0] != '\0' && !StringUtils::ContainsIgnoringCase(run.Task, m_search);
		if (isSearchingWithin)
		{
			ImGui::SetNextItemOpen(true);
		}
		const bool isOpen = ImGui::TreeNodeEx(run.Task.c_str(), ImGuiTreeNodeFlags_SpanAllColumns);

		const RunResult result = GetRunResult(tasks, run);
		ImGui::TableSetColumnIndex(1);
		ImGui::TextColored(result.Colour, "%s", result.Text.c_str());
		ImGui::TableSetColumnIndex(2);
		ImGui::TextDisabled("%s", TimeUtils::FormatClockTime(run.Entries.front()->Time).c_str());
		ImGui::TableSetColumnIndex(3);
		ImGui::TextDisabled("%s", TimeUtils::FormatDuration(GetSecondsTaken(tasks, run)).c_str());

		if (isOpen)
		{
			std::vector<const TaskTracker::HistoryEntry*> listed;
			for (const TaskTracker::HistoryEntry* const entry : run.Entries)
			{
				if (IsListed(run, *entry))
				{
					listed.push_back(entry);
				}
			}

			// A folder of thumbnails is an entry per image, so only those in view are drawn
			ImGuiListClipper clipper;
			clipper.Begin(static_cast<int>(listed.size()));
			while (clipper.Step())
			{
				for (int entryIndex = clipper.DisplayStart; entryIndex < clipper.DisplayEnd; ++entryIndex)
				{
					const TaskTracker::HistoryEntry& entry = *listed[entryIndex];
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::TextColored(TaskColours::ForHistory(entry.Kind), "%s", entry.Text.c_str());
					if (ImGui::IsItemHovered() && ImGui::CalcTextSize(entry.Text.c_str()).x > ImGui::GetColumnWidth())
					{
						ImGui::SetTooltip("%s", entry.Text.c_str());
					}
					ImGui::TableSetColumnIndex(2);
					ImGui::TextDisabled("%s", TimeUtils::FormatClockTime(entry.Time).c_str());
				}
			}
			ImGui::TreePop();
		}

		ImGui::PopID();
	}

	bool TaskHistoryPanel::IsShown(const TaskTracker::HistoryRun& run) const
	{
		if (m_isProblemsOnly && !run.HasProblems())
		{
			return false;
		}
		if (m_search[0] == '\0' || StringUtils::ContainsIgnoringCase(run.Task, m_search))
		{
			return true;
		}
		return std::any_of(run.Entries.begin(), run.Entries.end(),
			[this](const TaskTracker::HistoryEntry* entry) { return StringUtils::ContainsIgnoringCase(entry->Text, m_search); });
	}

	bool TaskHistoryPanel::IsListed(const TaskTracker::HistoryRun& run, const TaskTracker::HistoryEntry& entry) const
	{
		return m_search[0] == '\0' || StringUtils::ContainsIgnoringCase(run.Task, m_search) || StringUtils::ContainsIgnoringCase(entry.Text, m_search);
	}

	std::string TaskHistoryPanel::FormatAsText(const std::vector<const TaskTracker::HistoryRun*>& runs)
	{
		std::string text;
		for (const TaskTracker::HistoryRun* const run : runs)
		{
			text += run->Task + "\n";
			for (const TaskTracker::HistoryEntry* const entry : run->Entries)
			{
				text += "  " + TimeUtils::FormatClockTime(entry->Time) + "  " + entry->Text + "\n";
			}
		}
		return text;
	}
}
