#pragma once

#include <ImGui/imgui.h>

namespace Editor
{
	// Shared layout for groups of controls in a panel's menu bar: wide gaps between groups and tight
	// spacing within them, so each group reads as one unit
	namespace ToolbarLayout
	{
		// In pixels
		constexpr float k_itemSpacingInGroup = 6.0f;
		constexpr float k_groupGap = 12.0f;

		// Pushes the tight in-group spacing; pair with ImGui::PopStyleVar()
		inline void PushGroupSpacing()
		{
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(k_itemSpacingInGroup, ImGui::GetStyle().ItemSpacing.y));
		}

		// A vertical separator with a gap either side, before a group of controls
		inline void BeginGroup()
		{
			ImGui::Dummy(ImVec2(k_groupGap - k_itemSpacingInGroup, 0.0f));
			ImGui::Separator();
			ImGui::Dummy(ImVec2(k_groupGap - k_itemSpacingInGroup, 0.0f));
		}
	}
}
