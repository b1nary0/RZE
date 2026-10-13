#include <UI/Modals/UnsavedChangesPrompt.h>

#include <Services/SceneDocument.h>
#include <UI/Framework/EditorContext.h>

#include <ImGui/imgui.h>

namespace
{
	constexpr char k_popupID[] = "Unsaved Changes";
	constexpr float k_buttonWidth = 110.0f;
}

namespace Editor
{
	void UnsavedChangesPrompt::Request(EditorContext& context, std::string actionDescription, std::function<void()> action)
	{
		if (!context.GetSceneDocument().IsModified())
		{
			action();
			return;
		}

		m_actionDescription = std::move(actionDescription);
		m_pendingAction = std::move(action);
		// Opened from Display, since a request can come from inside a menu, whose ID stack the popup mustn't use.
		// While the save is being asked about, the new action just waits on it instead.
		m_shouldOpen = !m_isSaving;
	}

	void UnsavedChangesPrompt::Display(EditorContext& context)
	{
		if (m_shouldOpen)
		{
			ImGui::OpenPopup(k_popupID);
			m_shouldOpen = false;
		}

		const ImGuiViewport* const viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));

		const ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;
		if (!ImGui::BeginPopupModal(k_popupID, nullptr, flags))
		{
			return;
		}

		const SceneDocument& document = context.GetSceneDocument();
		const std::string sceneName = document.HasFilePath() ? document.GetFilePath().GetRelativePath() : std::string("The new scene");
		ImGui::Text("%s has unsaved changes.", sceneName.c_str());
		ImGui::Text("Save them before %s?", m_actionDescription.c_str());
		ImGui::Spacing();

		if (ImGui::Button("Save", ImVec2(k_buttonWidth, 0.0f)))
		{
			// Cancelling the file browser for a new scene cancels the whole thing, rather than losing the changes
			ImGui::CloseCurrentPopup();
			m_isSaving = true;
			context.GetSceneDocument().SaveWithPrompt([this](bool saved)
				{
					m_isSaving = false;
					Finish(saved);
				});
		}
		ImGui::SameLine();
		if (ImGui::Button("Don't Save", ImVec2(k_buttonWidth, 0.0f)))
		{
			ImGui::CloseCurrentPopup();
			Finish(true);
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(k_buttonWidth, 0.0f)) || ImGui::IsKeyPressed(ImGuiKey_Escape))
		{
			ImGui::CloseCurrentPopup();
			Finish(false);
		}

		ImGui::EndPopup();
	}

	void UnsavedChangesPrompt::Finish(bool runAction)
	{
		// Moved out first, so an action that requests again (or unloads the scene) starts from a clean state
		std::function<void()> action = std::move(m_pendingAction);
		m_pendingAction = nullptr;
		m_actionDescription.clear();

		if (runAction && action)
		{
			action();
		}
	}
}
