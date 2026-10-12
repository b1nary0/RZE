#include <UI/Framework/EditorUI.h>

#include <algorithm>

namespace
{
	// Toolbar spacing, in pixels
	constexpr float k_itemSpacingInGroup = 6.0f;
	constexpr float k_groupGap = 12.0f;

	// The label column never takes more than this share of a property table
	constexpr float k_maxLabelColumnFraction = 0.5f;
}

namespace Editor
{
	namespace EditorUI
	{
		void SectionHeading(const char* text)
		{
			ImGui::TextColored(k_headingColour, "%s", text);
			ImGui::Separator();
		}

		void ItemTooltip(const char* text)
		{
			if (text != nullptr && text[0] != '\0' && ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("%s", text);
			}
		}

		ScopedToolbarSpacing::ScopedToolbarSpacing()
		{
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(k_itemSpacingInGroup, ImGui::GetStyle().ItemSpacing.y));
		}

		ScopedToolbarSpacing::~ScopedToolbarSpacing()
		{
			ImGui::PopStyleVar();
		}

		void ToolbarGroupSeparator()
		{
			ImGui::Dummy(ImVec2(k_groupGap - k_itemSpacingInGroup, 0.0f));
			ImGui::Separator();
			ImGui::Dummy(ImVec2(k_groupGap - k_itemSpacingInGroup, 0.0f));
		}

		float CalcLabelColumnWidth(float widestLabelWidth)
		{
			const float padding = ImGui::GetStyle().CellPadding.x * 2.0f + ImGui::GetStyle().ItemSpacing.x;
			return std::min(widestLabelWidth + padding, ImGui::GetContentRegionAvail().x * k_maxLabelColumnFraction);
		}

		bool BeginPropertyTable(const char* id, float labelColumnWidth)
		{
			if (!ImGui::BeginTable(id, 2, ImGuiTableFlags_None))
			{
				return false;
			}

			ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, labelColumnWidth);
			ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
			return true;
		}

		void EndPropertyTable()
		{
			ImGui::EndTable();
		}

		void PropertyRow(const char* label, const char* tooltip)
		{
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted(label);

			if (tooltip != nullptr && tooltip[0] != '\0')
			{
				ItemTooltip(tooltip);
			}
			else if (ImGui::GetItemRectSize().x > ImGui::GetColumnWidth())
			{
				// Clipped by the column
				ItemTooltip(label);
			}

			ImGui::TableSetColumnIndex(1);
			ImGui::SetNextItemWidth(-FLT_MIN);
		}
	}
}
