#include <UI/Panels/LogFilter.h>

#include <UI/Framework/EditorUI.h>

#include <Utils/StringUtils.h>

#include <ImGui/imgui.h>

namespace Editor
{
	namespace
	{
		constexpr float k_searchWidth = 18.0f;

		struct ChannelToggle
		{
			LogChannel Channel;
			const char* Label;
			const char* Tooltip;
		};

		// Most severe first
		constexpr ChannelToggle k_channelToggles[] =
		{
			{ LogChannel::Error, "Error", "Something failed" },
			{ LogChannel::Warning, "Warning", "Something recovered or fell back" },
			{ LogChannel::Info, "Info", "Start-up, shutdown and one-off status" },
			{ LogChannel::Debug, "Debug", "High-volume detail, e.g. a line per resource" },
			{ LogChannel::Build, "Build", "The game's build and launch output" },
		};
	}

	LogFilter::LogFilter()
	{
		for (bool& isShown : m_isChannelShown)
		{
			isShown = true;
		}

		// Hidden until asked for, since it's most of the log
		m_isChannelShown[GetLogChannelIndex(LogChannel::Debug)] = false;
	}

	void LogFilter::Draw()
	{
		ImGui::SetNextItemWidth(ImGui::GetFontSize() * k_searchWidth);
		if (ImGui::InputTextWithHint("##search", "Search the log", m_searchBuffer, sizeof(m_searchBuffer)))
		{
			m_search = m_searchBuffer;
		}

		for (const ChannelToggle& toggle : k_channelToggles)
		{
			ImGui::SameLine();
			ImGui::Checkbox(toggle.Label, &m_isChannelShown[GetLogChannelIndex(toggle.Channel)]);
			EditorUI::ItemTooltip(toggle.Tooltip);
		}
	}

	bool LogFilter::IsShown(const DebugServices::LogEntry& entry) const
	{
		return m_isChannelShown[GetLogChannelIndex(entry.Channel)] && StringUtils::ContainsIgnoringCase(entry.Text, m_search);
	}
}
