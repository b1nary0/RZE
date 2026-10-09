#include <UI/Modals/AsyncOperationModal.h>

#include <EngineCore/Async/AsyncOperationManager.h>

#include <ImGui/imgui.h>

#include <stdio.h>

namespace
{
	constexpr char k_popupID[] = "##AsyncOperationModal";
	constexpr float k_modalWidth = 420.0f;
}

namespace Editor
{
	void AsyncOperationModal::Display(const AsyncOperationManager& operationManager)
	{
		const AsyncOperation* const operation = operationManager.GetBlockingOperation();

		if (operation != nullptr && !ImGui::IsPopupOpen(k_popupID))
		{
			ImGui::OpenPopup(k_popupID);
		}

		const ImGuiViewport* const viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
		// Zero height auto-fits to the content each frame.
		ImGui::SetNextWindowSize(ImVec2(k_modalWidth, 0.0f), ImGuiCond_Always);

		const ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar
			| ImGuiWindowFlags_NoMove
			| ImGuiWindowFlags_NoResize
			| ImGuiWindowFlags_NoSavedSettings;

		// No p_open, so the user can't close it (Escape included); it closes when the operation finishes.
		if (ImGui::BeginPopupModal(k_popupID, nullptr, flags))
		{
			if (operation == nullptr)
			{
				ImGui::CloseCurrentPopup();
			}
			else
			{
				ImGui::TextUnformatted(operation->GetName().c_str());
				ImGui::Spacing();

				const float progress = operation->GetProgress();
				char overlay[16];
				snprintf(overlay, sizeof(overlay), "%d%%", static_cast<int>(progress * 100.0f));
				ImGui::ProgressBar(progress, ImVec2(-FLT_MIN, 0.0f), overlay);

				const std::string statusText = operation->GetStatusText();
				if (!statusText.empty())
				{
					ImGui::Spacing();
					ImGui::PushTextWrapPos(0.0f);
					ImGui::TextDisabled("%s", statusText.c_str());
					ImGui::PopTextWrapPos();
				}
			}

			ImGui::EndPopup();
		}
	}
}
