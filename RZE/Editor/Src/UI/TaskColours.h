#pragma once

#include <UI/TaskTracker.h>

#include <ImGui/imgui.h>

namespace Editor
{
	// The colours that say how background tasks are going, shared by the status bar and the Task History window
	namespace TaskColours
	{
		constexpr ImVec4 k_ok = ImVec4(0.40f, 0.80f, 0.45f, 1.0f);
		constexpr ImVec4 k_running = ImVec4(0.95f, 0.80f, 0.25f, 1.0f);
		constexpr ImVec4 k_warning = ImVec4(1.0f, 0.60f, 0.20f, 1.0f);
		constexpr ImVec4 k_error = ImVec4(1.0f, 0.42f, 0.36f, 1.0f);

		ImVec4 ForStatus(ETaskStatus status);
		// A history entry's text: grey for when it started, plain for a step, and how it ended or warned in its colour
		ImVec4 ForHistory(EHistoryKind kind);
	}
}
