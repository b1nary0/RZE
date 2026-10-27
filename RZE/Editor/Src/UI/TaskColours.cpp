#include <UI/TaskColours.h>

namespace Editor
{
	namespace TaskColours
	{
		ImVec4 ForStatus(ETaskStatus status)
		{
			switch (status)
			{
			case ETaskStatus::Running:
				return EditorStyle::k_runningColour;
			case ETaskStatus::Warning:
				return EditorStyle::k_warningColour;
			case ETaskStatus::Failed:
				return EditorStyle::k_errorColour;
			default:
				return EditorStyle::k_okColour;
			}
		}

		ImVec4 ForHistory(EHistoryKind kind)
		{
			switch (kind)
			{
			case EHistoryKind::Started:
				return ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled);
			case EHistoryKind::Finished:
				return EditorStyle::k_okColour;
			case EHistoryKind::Warning:
			case EHistoryKind::FinishedWithWarnings:
			case EHistoryKind::Cancelled:
				return EditorStyle::k_warningColour;
			case EHistoryKind::Failed:
				return EditorStyle::k_errorColour;
			default:
				return ImGui::GetStyleColorVec4(ImGuiCol_Text);
			}
		}
	}
}
