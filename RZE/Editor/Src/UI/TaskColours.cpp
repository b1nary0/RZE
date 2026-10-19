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
				return k_running;
			case ETaskStatus::Warning:
				return k_warning;
			case ETaskStatus::Failed:
				return k_error;
			default:
				return k_ok;
			}
		}

		ImVec4 ForHistory(EHistoryKind kind)
		{
			switch (kind)
			{
			case EHistoryKind::Started:
				return ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled);
			case EHistoryKind::Finished:
				return k_ok;
			case EHistoryKind::Warning:
			case EHistoryKind::FinishedWithWarnings:
			case EHistoryKind::Cancelled:
				return k_warning;
			case EHistoryKind::Failed:
				return k_error;
			default:
				return ImGui::GetStyleColorVec4(ImGuiCol_Text);
			}
		}
	}
}
