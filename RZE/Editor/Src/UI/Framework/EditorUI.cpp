#include <UI/Framework/EditorUI.h>

#include <Rendering/Renderer.h>

#include <Utils/Math/Math.h>

#include <ImGui/imgui_internal.h> // ImGui::GetCurrentTable

#include <algorithm>
#include <cmath>
#include <cstdarg>

namespace
{
	// Toolbar spacing, in pixels
	constexpr float k_itemSpacingInGroup = 6.0f;
	constexpr float k_groupGap = 12.0f;

	// The label column never takes more than this share of a property table
	constexpr float k_maxLabelColumnFraction = 0.5f;

	// A property section's rule, quieter than a full separator
	constexpr ImVec4 k_sectionLineColour = ImVec4(1.0f, 1.0f, 1.0f, 0.12f);

	// The draw list channels around a tree node: its highlight is drawn once ImGui has drawn the node, but goes behind it
	constexpr int k_highlightChannel = 0;
	constexpr int k_contentChannel = 1;

	// Shared by every tree node, which merges it before the next one splits it, so its memory is reused
	ImDrawListSplitter& GetTreeNodeSplitter()
	{
		static ImDrawListSplitter splitter;
		return splitter;
	}

	// The right edge of the property table being drawn
	float GetPropertyRowRight()
	{
		return ImGui::GetCurrentTable()->WorkRect.Max.x;
	}
}

namespace Editor
{
	namespace EditorUI
	{
		ImTextureID TextureID(const Rendering::TextureBuffer2DHandle& texture)
		{
			Rendering::Renderer::KeepTextureForUI(texture);
			return (ImTextureID)(intptr_t)texture.GetTextureData();
		}

		void PushPropertyRowClipRect(float bottom)
		{
			// Not intersected with the current clip rect, which is the label column's, but with the table's visible area
			const ImRect& tableClip = ImGui::GetCurrentTable()->InnerClipRect;
			const ImVec2 start = ImGui::GetCursorScreenPos();
			ImGui::PushClipRect(ImVec2(std::max(start.x, tableClip.Min.x), std::max(start.y, tableClip.Min.y)),
				ImVec2(std::min(GetPropertyRowRight(), tableClip.Max.x), std::min(bottom, tableClip.Max.y)), false);
		}

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
			case EIcon::Pin:
			case EIcon::Unpin:
			{
				// A pin standing up with its point down, or for Unpin lying on its side with its point to the left: the
				// head above a crossbar, and the needle below it. Laid out standing, then turned.
				const bool isLying = icon == EIcon::Unpin;
				auto place = [&](float x, float y)
					{
						return isLying ? ImVec2(centre.x - y * radius, centre.y + x * radius) : ImVec2(centre.x + x * radius, centre.y + y * radius);
					};

				const float headHalfWidth = 0.4f;
				const float crossbarHalfWidth = 0.85f;
				const float crossbarY = 0.15f;
				drawList->AddQuad(place(-headHalfWidth, -1.0f), place(headHalfWidth, -1.0f), place(headHalfWidth, crossbarY),
					place(-headHalfWidth, crossbarY), colour, thickness);
				drawList->AddLine(place(-crossbarHalfWidth, crossbarY), place(crossbarHalfWidth, crossbarY), colour, thickness);
				drawList->AddLine(place(0.0f, crossbarY), place(0.0f, 1.0f), colour, thickness);
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

		bool MenuItem(const char* label, const char* shortcut, bool enabled)
		{
			ImGuiWindow* const window = ImGui::GetCurrentWindow();
			const float textY = ImGui::GetCursorScreenPos().y + window->DC.CurrLineTextBaseOffset;
			const bool isPressed = ImGui::MenuItem(label, nullptr, false, enabled);
			if (shortcut == nullptr || shortcut[0] == '\0')
			{
				return isPressed;
			}

			// Room for it beside the widest label, as ImGui makes for its own shortcuts; the menu is that wide next frame
			const float shortcutWidth = ImGui::CalcTextSize(shortcut).x;
			window->DC.MenuColumns.DeclColumns(0.0f, 0.0f, shortcutWidth, 0.0f);

			if (ImGui::IsItemVisible())
			{
				// Faded further while the item's disabled, like its label
				ImGui::BeginDisabled(!enabled);
				window->DrawList->AddText(ImVec2(window->WorkRect.Max.x - shortcutWidth, textY), ImGui::GetColorU32(ImGuiCol_TextDisabled), shortcut);
				ImGui::EndDisabled();
			}
			return isPressed;
		}

		bool TreeNode(const char* id, ImGuiTreeNodeFlags flags, const char* format, ...)
		{
			// A node across a table's columns is highlighted in the table's background, which is behind them all. Any
			// other is drawn on its own channel, so the highlight can go behind it.
			const bool spansColumns = (flags & ImGuiTreeNodeFlags_SpanAllColumns) != 0 && ImGui::GetCurrentTable() != nullptr;
			ImDrawList* const drawList = ImGui::GetWindowDrawList();
			ImDrawListSplitter& splitter = GetTreeNodeSplitter();
			if (!spansColumns)
			{
				splitter.Split(drawList, 2);
				splitter.SetCurrentChannel(drawList, k_contentChannel);
			}

			// In place of ImGui's own highlight
			ImGui::PushStyleColor(ImGuiCol_Header, IM_COL32_BLACK_TRANS);
			ImGui::PushStyleColor(ImGuiCol_HeaderHovered, IM_COL32_BLACK_TRANS);
			ImGui::PushStyleColor(ImGuiCol_HeaderActive, IM_COL32_BLACK_TRANS);
			va_list args;
			va_start(args, format);
			const bool isOpen = ImGui::TreeNodeExV(id, flags, format, args);
			va_end(args);
			ImGui::PopStyleColor(3);

			const bool isHovered = ImGui::IsItemHovered();
			const bool isSelected = (flags & ImGuiTreeNodeFlags_Selected) != 0;
			if (spansColumns)
			{
				ImGui::TablePushBackgroundChannel();
			}
			else
			{
				splitter.SetCurrentChannel(drawList, k_highlightChannel);
			}

			if (ImGui::IsItemVisible() && (isHovered || isSelected))
			{
				// The whole row, which can be wider than the part that's hovered
				const ImRect& rect = ImGui::GetCurrentContext()->LastItemData.DisplayRect;
				const ImGuiCol colour = isHovered ? (ImGui::IsItemActive() ? ImGuiCol_HeaderActive : ImGuiCol_HeaderHovered) : ImGuiCol_Header;
				drawList->AddRectFilled(rect.Min, rect.Max, ImGui::GetColorU32(colour), k_cornerRounding);
			}

			if (spansColumns)
			{
				ImGui::TablePopBackgroundChannel();
			}
			else
			{
				splitter.Merge(drawList);
			}
			return isOpen;
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
