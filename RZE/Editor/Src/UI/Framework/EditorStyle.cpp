#include <UI/Framework/EditorStyle.h>

namespace
{
	// colour with its alpha replaced
	ImVec4 WithAlpha(const ImVec4& colour, float alpha)
	{
		return ImVec4(colour.x, colour.y, colour.z, alpha);
	}
}

namespace Editor
{
	namespace EditorStyle
	{
		void Apply()
		{
			ImGuiStyle& style = ImGui::GetStyle();
			// Everything ImGui draws with corners. Scrollbars and tabs keep ImGui's own, rounder ones.
			style.WindowRounding = k_cornerRounding;
			style.ChildRounding = k_cornerRounding;
			style.PopupRounding = k_cornerRounding;
			style.FrameRounding = k_cornerRounding;
			style.GrabRounding = k_cornerRounding;
			style.ImageRounding = k_cornerRounding;
			style.MenuItemRounding = k_cornerRounding;
			style.SelectableRounding = k_cornerRounding;

			style.Colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
			style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
			style.Colors[ImGuiCol_WindowBg] = ImVec4(0.13f, 0.13f, 0.13f, 1.00f);
			style.Colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
			style.Colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
			style.Colors[ImGuiCol_Border] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
			style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
			style.Colors[ImGuiCol_FrameBg] = ImVec4(0.46f, 0.46f, 0.46f, 0.54f);
			style.Colors[ImGuiCol_FrameBgHovered] = WithAlpha(k_accentColour, 0.40f);
			style.Colors[ImGuiCol_FrameBgActive] = WithAlpha(k_accentColour, 0.67f);
			style.Colors[ImGuiCol_TitleBg] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
			style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.23f, 0.23f, 0.23f, 1.00f);
			style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
			style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
			style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
			style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
			style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
			style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
			style.Colors[ImGuiCol_CheckMark] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
			style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
			style.Colors[ImGuiCol_SliderGrabActive] = k_accentColour;
			style.Colors[ImGuiCol_Button] = ImVec4(0.67f, 0.67f, 0.67f, 0.40f);
			style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
			style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
			style.Colors[ImGuiCol_Header] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
			style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
			style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
			style.Colors[ImGuiCol_Separator] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
			style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
			style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
			style.Colors[ImGuiCol_ResizeGrip] = WithAlpha(k_accentColour, 0.25f);
			style.Colors[ImGuiCol_ResizeGripHovered] = WithAlpha(k_accentColour, 0.67f);
			style.Colors[ImGuiCol_ResizeGripActive] = WithAlpha(k_accentColour, 0.95f);
			style.Colors[ImGuiCol_Tab] = ImVec4(0.17f, 0.17f, 0.17f, 0.86f);
			style.Colors[ImGuiCol_TabHovered] = WithAlpha(k_accentColour, 0.80f);
			// Blue for the focused panel's tab, grey for the tabs other panels are showing
			style.Colors[ImGuiCol_TabSelected] = ImVec4(0.14f, 0.26f, 0.42f, 1.00f);
			style.Colors[ImGuiCol_TabDimmed] = ImVec4(0.07f, 0.10f, 0.15f, 0.97f);
			style.Colors[ImGuiCol_TabDimmedSelected] = ImVec4(0.37f, 0.37f, 0.37f, 1.00f);
			style.Colors[ImGuiCol_DockingPreview] = WithAlpha(k_accentColour, 0.70f);
			style.Colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
			style.Colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
			style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
			style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
			style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
			style.Colors[ImGuiCol_TextSelectedBg] = WithAlpha(k_accentColour, 0.35f);
			style.Colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
			style.Colors[ImGuiCol_NavHighlight] = k_accentColour;
			style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
			style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
			style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
		}
	}
}
