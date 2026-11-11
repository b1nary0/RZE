#pragma once

#include <imgui/imgui.h>

// The editor's look: the colours and metrics its panels share, and ImGui's style set from them
namespace Editor
{
	namespace EditorStyle
	{
		// Section headings, e.g. each component in the Object Inspector
		constexpr ImVec4 k_headingColour = ImVec4(0.65f, 0.65f, 1.0f, 1.0f);

		// What's selected or confirms, e.g. a selected tile's outline or a dialog's main button
		constexpr ImVec4 k_accentColour = ImVec4(0.26f, 0.59f, 0.98f, 1.0f);
		constexpr ImVec4 k_accentHoveredColour = ImVec4(0.36f, 0.66f, 1.0f, 1.0f);
		constexpr ImVec4 k_accentActiveColour = ImVec4(0.18f, 0.48f, 0.86f, 1.0f);

		// A label's usual colour: softer than the values, so the eye goes to what can be changed
		constexpr ImVec4 k_labelColour = ImVec4(0.70f, 0.71f, 0.75f, 1.0f);

		// How something is going, e.g. a task (TaskColours), or how full the render command memory is
		constexpr ImVec4 k_okColour = ImVec4(0.40f, 0.80f, 0.45f, 1.0f);
		// Still going, or getting full
		constexpr ImVec4 k_runningColour = ImVec4(0.95f, 0.80f, 0.25f, 1.0f);
		constexpr ImVec4 k_warningColour = ImVec4(1.0f, 0.60f, 0.20f, 1.0f);
		// Failed or full, and what's wrong with something typed, e.g. a file name that can't be used
		constexpr ImVec4 k_errorColour = ImVec4(1.0f, 0.42f, 0.36f, 1.0f);

		// How much the corners of windows, popups, fields and highlights are rounded, at 96 DPI: just enough to soften them
		constexpr float k_cornerRounding = 3.0f;

		// Sets ImGui's style: its colours and corner rounding. Once, after ImGui's context is made.
		void Apply();
	}
}
