#pragma once

#include <ImGui/imgui.h>

namespace Rendering
{
	class TextureBuffer2DHandle;
}

// Small ImGui helpers shared by the editor's panels, so the same kinds of controls look and behave the same way
// everywhere
namespace Editor
{
	namespace EditorUI
	{
		// Section headings, e.g. each component in the Object Inspector
		constexpr ImVec4 k_headingColour = ImVec4(0.65f, 0.65f, 1.0f, 1.0f);

		// What's selected or confirms, e.g. a selected tile's outline or a dialog's main button
		constexpr ImVec4 k_accentColour = ImVec4(0.26f, 0.59f, 0.98f, 1.0f);
		constexpr ImVec4 k_accentHoveredColour = ImVec4(0.36f, 0.66f, 1.0f, 1.0f);
		constexpr ImVec4 k_accentActiveColour = ImVec4(0.18f, 0.48f, 0.86f, 1.0f);

		// What's wrong with something typed, e.g. a file name that can't be used
		constexpr ImVec4 k_errorColour = ImVec4(1.0f, 0.45f, 0.40f, 1.0f);

		// How much the corners of windows, popups, fields and highlights are rounded, at 96 DPI: just enough to soften them
		constexpr float k_cornerRounding = 3.0f;

		// colour with its alpha replaced, packed for an ImDrawList
		ImU32 WithAlpha(const ImVec4& colour, float alpha);

		// A button in the accent colours, for a dialog's main action (Open, Save)
		bool PrimaryButton(const char* label, const ImVec2& size = ImVec2(0.0f, 0.0f));

		// ImGui::MenuItem, with its shortcut (e.g. "Ctrl+Z"; none if null or empty) ending at the menu's right edge,
		// inset as far as the labels are from its left. ImGui's own lines shortcuts up on their left instead.
		bool MenuItem(const char* label, const char* shortcut, bool enabled = true);

		// ImGui::TreeNodeEx, with its highlight's corners rounded like everything else's. ImGui always draws an unframed
		// node's highlight square, so the editor's tree nodes use this instead. It reads the node's row from ImGui's
		// internals (LastItemData.DisplayRect, TablePushBackgroundChannel), which an ImGui upgrade may change.
		bool TreeNode(const char* id, ImGuiTreeNodeFlags flags, const char* format, ...) IM_FMTARGS(3);

		// Text centred on centre, in the current font at fontSize
		void AddCentredText(ImDrawList* drawList, const ImVec2& centre, float fontSize, ImU32 colour, const char* text);

		// The ID ImGui draws texture by, for ImGui::Image, ImDrawList::AddImage and so on. The UI drawn this frame keeps
		// texture alive for as long as it's rendered, so draw textures only through this.
		ImTextureID TextureID(const Rendering::TextureBuffer2DHandle& texture);

		// A heading in k_headingColour with a separator under it
		void SectionHeading(const char* text);

		// Shows text as a tooltip while the last item is hovered; does nothing for null or empty text
		void ItemTooltip(const char* text);

		// Small icons drawn with lines, since the editor's fonts only have Latin glyphs
		enum class EIcon
		{
			// A circular arrow: back to how it was
			Reset,
			// A cross: empty it
			Clear,
			// Chevrons pointing left, right and up: back, forward and up a level
			Back,
			Forward,
			Up,
			// A pin standing up: stays open. Lying on its side: hides itself when not in use.
			Pin,
			Unpin
		};

		// A square, frame-height button showing icon, with tooltip on hover; true on the frame it's clicked
		bool IconButton(const char* id, EIcon icon, const char* tooltip = nullptr);
		// The width an IconButton takes
		float GetIconButtonWidth();

		// A radio button that selects option into value; true on the frame it's clicked
		template <typename T>
		bool RadioOption(const char* label, T& value, T option)
		{
			if (ImGui::RadioButton(label, value == option))
			{
				value = option;
				return true;
			}
			return false;
		}

		//
		// Toolbars (a panel's menu bar): controls in groups, with wide gaps between groups and tight spacing within
		//

		// Tight spacing between the controls in a group, for as long as it's in scope
		class ScopedToolbarSpacing
		{
		public:
			ScopedToolbarSpacing();
			~ScopedToolbarSpacing();

			ScopedToolbarSpacing(const ScopedToolbarSpacing&) = delete;
			ScopedToolbarSpacing& operator=(const ScopedToolbarSpacing&) = delete;
		};

		// A vertical separator with a gap either side, before a group of controls
		void ToolbarGroupSeparator();

		//
		// Property tables: a label column and a value column, as in the Object Inspector
		//

		// The label column's width for labels as wide as widestLabelWidth (from ImGui::CalcTextSize), capped so the
		// values keep at least half the space
		float CalcLabelColumnWidth(float widestLabelWidth);

		// Pair with EndPropertyTable if it returns true
		bool BeginPropertyTable(const char* id, float labelColumnWidth);
		void EndPropertyTable();

		// A label's usual colour: softer than the values, so the eye goes to what can be changed
		constexpr ImVec4 k_labelColour = ImVec4(0.70f, 0.71f, 0.75f, 1.0f);

		// Starts a row, draws its label in labelColour (shown in full on hover when it doesn't fit, or tooltip if
		// there is one) and moves to the value column, sized for a full-width widget
		void PropertyRow(const char* label, const char* tooltip = nullptr, const ImVec4& labelColour = k_labelColour);

		// A row across both columns that starts a section of related rows: title, then a line to the row's end. An
		// empty title draws just the line, e.g. to end a section before rows that aren't in one.
		void PropertySectionRow(const char* title);

		// Lets what's drawn next in a property table's label column run across the row to its right edge, and down to
		// bottom, while keeping it inside the table's visible area. Pair with ImGui::PopClipRect.
		void PushPropertyRowClipRect(float bottom);

		// Starts a row whose content runs across both columns (e.g. an image) and returns its width. Pair with
		// EndPropertyWideRow.
		float BeginPropertyWideRow();
		void EndPropertyWideRow();
	}
}
