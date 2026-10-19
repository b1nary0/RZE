#include <UI/Settings/BackgroundTasksSettingsPage.h>

#include <UI/Settings/SettingsPageRegistry.h>

#include <RZE.h>

#include <EngineCore/Async/AsyncOperationFilter.h>
#include <EngineCore/Async/AsyncOperationManager.h>

#include <ImGui/imgui.h>

namespace
{
	// A checkbox with a tooltip. True when it was changed.
	bool DrawSwitch(const char* label, const char* tooltip, bool& isShown)
	{
		const bool isChanged = ImGui::Checkbox(label, &isShown);
		ImGui::SetItemTooltip("%s", tooltip);
		return isChanged;
	}

	// A checkbox for one kind's bit of shownKinds, with a tooltip. True when it was changed.
	bool DrawKindSwitch(const char* label, const char* tooltip, EAsyncOperationKind kind, U32& shownKinds)
	{
		const bool isChanged = ImGui::CheckboxFlags(label, &shownKinds, static_cast<U32>(kind));
		ImGui::SetItemTooltip("%s", tooltip);
		return isChanged;
	}
}

namespace Editor
{
	REGISTER_SETTINGS_PAGE(BackgroundTasksSettingsPage);

	void BackgroundTasksSettingsPage::Draw(EditorContext&)
	{
		AsyncOperationManager& operationManager = RZE().GetAsyncOperationManager();
		AsyncOperationFilter filter = operationManager.GetVisibilityFilter();
		bool isChanged = false;

		ImGui::TextUnformatted("The status bar shows");
		ImGui::Spacing();

		isChanged |= DrawSwitch("Steps of other tasks", "Tasks that are part of another one, which reports on them too, e.g. the scene a thumbnail run loads", filter.ShowSubSteps);
		isChanged |= DrawSwitch("Tasks with a progress dialog", "Tasks that stop you using the editor until they're done, e.g. opening a scene", filter.ShowBlocking);

		ImGui::Spacing();
		ImGui::SeparatorText("Kinds");
		isChanged |= DrawKindSwitch("Resource loads", "Meshes, materials and images loading, e.g. the Content Browser's thumbnails", EAsyncOperationKind::ResourceLoad, filter.ShownKinds);
		isChanged |= DrawKindSwitch("Scene loads", "Scenes loading", EAsyncOperationKind::SceneLoad, filter.ShownKinds);
		isChanged |= DrawKindSwitch("Rendering", "Images being rendered, e.g. scene and mesh thumbnails", EAsyncOperationKind::Rendering, filter.ShownKinds);
		isChanged |= DrawKindSwitch("Saving", "Files being written, e.g. thumbnail images", EAsyncOperationKind::Saving, filter.ShownKinds);
		isChanged |= DrawKindSwitch("Other", "Everything else, e.g. building the game", EAsyncOperationKind::General, filter.ShownKinds);

		if (isChanged)
		{
			operationManager.SetVisibilityFilter(filter);
		}

		ImGui::Spacing();
		ImGui::TextDisabled("These last until the editor closes; Engine.ini's [BackgroundTasks] sets them at start-up.");
		ImGui::TextDisabled("A task that's hidden still runs, and comes back in the status bar if it's shown again.");
	}
}
