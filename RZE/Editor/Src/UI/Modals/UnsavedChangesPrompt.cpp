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
	struct UnsavedChangesPrompt::PendingRequest
	{
		// Finishes "Save them before ...", e.g. "exiting"
		std::string ActionDescription;
		std::function<void()> Action;
		// Opens the popup on the next Display
		bool ShouldOpen = false;
	};

	UnsavedChangesPrompt::UnsavedChangesPrompt() = default;
	UnsavedChangesPrompt::~UnsavedChangesPrompt() = default;

	void UnsavedChangesPrompt::Request(EditorContext& context, std::string actionDescription, std::function<void()> action)
	{
		if (!context.GetSceneDocument().IsModified())
		{
			action();
			return;
		}

		m_request = std::make_unique<PendingRequest>();
		m_request->ActionDescription = std::move(actionDescription);
		m_request->Action = std::move(action);
		// Opened from Display, since a request can come from inside a menu, whose ID stack the popup mustn't use.
		// While the save is being asked about, the new action just waits on it instead.
		m_request->ShouldOpen = !m_isSaving;
	}

	void UnsavedChangesPrompt::Display(EditorContext& context)
	{
		if (m_request != nullptr && m_request->ShouldOpen)
		{
			ImGui::OpenPopup(k_popupID);
			m_request->ShouldOpen = false;
		}

		const ImGuiViewport* const viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));

		const ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;
		if (!ImGui::BeginPopupModal(k_popupID, nullptr, flags))
		{
			return;
		}

		// Nothing left to ask about; every answer closes the popup too, so this is only a safeguard
		if (m_request == nullptr)
		{
			ImGui::CloseCurrentPopup();
			ImGui::EndPopup();
			return;
		}

		const SceneDocument& document = context.GetSceneDocument();
		const std::string sceneName = document.HasFilePath() ? document.GetFilePath().GetRelativePath() : std::string("The new scene");
		ImGui::Text("%s has unsaved changes.", sceneName.c_str());
		ImGui::Text("Save them before %s?", m_request->ActionDescription.c_str());
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
		const std::unique_ptr<PendingRequest> request = std::move(m_request);

		if (runAction && request != nullptr && request->Action)
		{
			request->Action();
		}
	}
}
