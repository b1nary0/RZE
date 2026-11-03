#pragma once

#include <UI/Framework/EditorStyle.h>
#include <UI/TaskTracker.h>

#include <imgui/imgui.h>

namespace Editor
{
	// Which of the status colours (EditorStyle) says how background tasks are going, for the status bar and the Task
	// History window
	namespace TaskColours
	{
		ImVec4 ForStatus(ETaskStatus status);
		// A history entry's text: grey for when it started, plain for a step, and how it ended or warned in its colour
		ImVec4 ForHistory(EHistoryKind kind);
	}
}
