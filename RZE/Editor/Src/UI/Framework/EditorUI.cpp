#include <UI/Framework/EditorUI.h>

#include <Utils/Math/Math.h>

#include <ImGui/imgui_internal.h> // ImGui::GetCurrentTable

#include <algorithm>
#include <cmath>

namespace
{
	// Toolbar spacing, in pixels
	constexpr float k_itemSpacingInGroup = 6.0f;
	constexpr float k_groupGap = 12.0f;

	// The label column never takes more than this share of a property table
	constexpr float k_maxLabelColumnFraction = 0.5f;

	// A property section's rule, quieter than a full separator
	constexpr ImVec4 k_sectionLineColour = ImVec4(1.0f, 1.0f, 1.0f, 0.12f);

	// The right edge of the property table being drawn
	float GetPropertyRowRight()
	{
		return ImGui::GetCurrentTable()->WorkRect.Max.x;
	}

	// Lets what's drawn next in a property table's label column run across the row to its right edge, and down to
	// bottom, while keeping it inside the window. Pair with ImGui::PopClipRect.
	void PushPropertyRowClipRect(float bottom)
	{
		// Not intersected with the current clip rect, which is the label column's, but with the table's visible area
		const ImRect& tableClip = ImGui::GetCurrentTable()->InnerClipRect;
		const ImVec2 start = ImGui::GetCursorScreenPos();
		ImGui::PushClipRect(ImVec2(std::max(start.x, tableClip.Min.x), std::max(start.y, tableClip.Min.y)),
			ImVec2(std::min(GetPropertyRowRight(), tableClip.Max.x), std::min(bottom, tableClip.Max.y)), false);
	}
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

		bool IconButton(const char* id, EIcon icon, const char* tooltip)
		{
			const float size = GetIconButtonWidth();
			const bool isClicked = ImGui::InvisibleButton(id, ImVec2(size, size));
			const bool isHovered = ImGui::IsItemHovered();
			ItemTooltip(tooltip);

			ImDrawList* const drawList = ImGui::GetWindowDrawList();
			const ImVec2 min = ImGui::GetItemRectMin();
			const ImVec2 max = ImGui::GetItemRectMax();
			if (isHovered)
			{
				drawList->AddRectFilled(min, max, ImGui::GetColorU32(ImGuiCol_ButtonHovered), ImGui::GetStyle().FrameRounding);
			}

			const ImU32 colour = ImGui::GetColorU32(isHovered ? k_accentHoveredColour : ImGui::GetStyleColorVec4(ImGuiCol_Text));
			const ImVec2 centre((min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f);
			const float radius = size * 0.26f;
			const float thickness = 1.5f;

			switch (icon)
			{
			case EIcon::Reset:
			{
				// Three quarters of a circle from the top round clockwise to the left, with an arrowhead at the top
				// pointing back (anticlockwise)
				const float start = -MathUtils::Pi * 0.5f;
				drawList->PathArcTo(centre, radius, start, start + MathUtils::Pi * 1.5f, 16);
				drawList->PathStroke(colour, ImDrawFlags_None, thickness);

				const ImVec2 top(centre.x, centre.y - radius);
				const float head = radius * 0.7f;
				drawList->AddTriangleFilled(ImVec2(top.x - head, top.y), ImVec2(top.x + head * 0.35f, top.y - head * 0.65f),
					ImVec2(top.x + head * 0.35f, top.y + head * 0.65f), colour);
				break;
			}
			case EIcon::Clear:
			{
				const float arm = radius * 0.85f;
				drawList->AddLine(ImVec2(centre.x - arm, centre.y - arm), ImVec2(centre.x + arm, centre.y + arm), colour, thickness);
				drawList->AddLine(ImVec2(centre.x - arm, centre.y + arm), ImVec2(centre.x + arm, centre.y - arm), colour, thickness);
				break;
			}
			case EIcon::Back:
			case EIcon::Forward:
			case EIcon::Up:
			{
				// Two strokes meeting at the tip, which points the icon's way
				const float arm = radius * 0.8f;
				const float half = arm * 0.5f;
				ImVec2 points[3];
				if (icon == EIcon::Up)
				{
					points[0] = ImVec2(centre.x - arm, centre.y + half);
					points[1] = ImVec2(centre.x, centre.y - half);
					points[2] = ImVec2(centre.x + arm, centre.y + half);
				}
				else
				{
					const float direction = icon == EIcon::Back ? -1.0f : 1.0f;
					points[0] = ImVec2(centre.x - half * direction, centre.y - arm);
					points[1] = ImVec2(centre.x + half * direction, centre.y);
					points[2] = ImVec2(centre.x - half * direction, centre.y + arm);
				}
				drawList->AddPolyline(points, 3, colour, ImDrawFlags_None, thickness);
				break;
			}
			}

			return isClicked;
		}

		float GetIconButtonWidth()
		{
			return ImGui::GetFrameHeight();
		}

		ImU32 WithAlpha(const ImVec4& colour, float alpha)
		{
			return ImGui::GetColorU32(ImVec4(colour.x, colour.y, colour.z, alpha));
		}

		bool PrimaryButton(const char* label, const ImVec2& size)
		{
			ImGui::PushStyleColor(ImGuiCol_Button, k_accentColour);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, k_accentHoveredColour);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, k_accentActiveColour);
			const bool isPressed = ImGui::Button(label, size);
			ImGui::PopStyleColor(3);
			return isPressed;
		}

		void AddCentredText(ImDrawList* drawList, const ImVec2& centre, float fontSize, ImU32 colour, const char* text)
		{
			ImFont* const font = ImGui::GetFont();
			const ImVec2 size = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, text);
			drawList->AddText(font, fontSize, ImVec2(centre.x - size.x * 0.5f, centre.y - size.y * 0.5f), colour, text);
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

		void PropertyRow(const char* label, const char* tooltip, const ImVec4& labelColour)
		{
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::AlignTextToFramePadding();
			ImGui::PushStyleColor(ImGuiCol_Text, labelColour);
			ImGui::TextUnformatted(label);
			ImGui::PopStyleColor();

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

		void PropertySectionRow(const char* title)
		{
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::AlignTextToFramePadding();

			const ImVec2 start = ImGui::GetCursorScreenPos();
			const float right = GetPropertyRowRight();
			// The text sits FramePadding.y down, so this is as tall as a widget row
			const float rowHeight = ImGui::GetFrameHeight();
			PushPropertyRowClipRect(start.y + rowHeight);

			float lineStart = start.x;
			if (title[0] != '\0')
			{
				ImGui::TextDisabled("%s", title);
				lineStart = ImGui::GetItemRectMax().x + ImGui::GetStyle().ItemSpacing.x;
			}
			else
			{
				ImGui::Dummy(ImVec2(0.0f, rowHeight));
			}

			const float lineY = std::floor(start.y + rowHeight * 0.5f) + 0.5f;
			ImGui::GetWindowDrawList()->AddLine(ImVec2(lineStart, lineY), ImVec2(right, lineY), ImGui::GetColorU32(k_sectionLineColour));
			ImGui::PopClipRect();
		}

		float BeginPropertyWideRow()
		{
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			PushPropertyRowClipRect(FLT_MAX);
			return GetPropertyRowRight() - ImGui::GetCursorScreenPos().x;
		}

		void EndPropertyWideRow()
		{
			ImGui::PopClipRect();
		}
	}
}
