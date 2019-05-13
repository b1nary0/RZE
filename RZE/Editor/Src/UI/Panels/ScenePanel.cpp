#include <UI/Panels/ScenePanel.h>

#include <EditorApp.h>

#include <ImGui/imgui.h>

namespace Editor
{

	ScenePanel::ScenePanel()
		: bEnabled(false)
	{
	}

	ScenePanel::~ScenePanel()
	{
	}

	void ScenePanel::Display()
	{
		if (ImGui::Begin("Scene", &bEnabled))
		{
			const std::vector<GameScene::SceneEntryTemp> entities = RZE_Application::RZE().GetActiveScene().GetSceneEntries();
			for (const GameScene::SceneEntryTemp& entry : entities)
			{
				if (ImGui::Selectable(entry.Name.c_str()))
				{
					Apollo::EntityHandler::ComponentNameIDMap componentMap;
					RZE_Application::RZE().GetActiveScene().GetEntityHandler().GetComponentNames(entry.ID, componentMap);

					mSelectedComponents.clear();
					for (auto& pair : componentMap)
					{
						mSelectedComponents.push_back(pair.second);
					}
				}
			}
		}
		ImGui::End();

		if (ImGui::Begin("Component View"))
		{
			for (auto& componentName : mSelectedComponents)
			{
				if (ImGui::Selectable(componentName.c_str()))
				{

				}
			}
		}
		ImGui::End();
	}

	void ScenePanel::Enable()
	{
		bEnabled = true;
	}

	void ScenePanel::Disable()
	{
		bEnabled = false;
	}

	bool ScenePanel::IsEnabled()
	{
		return bEnabled;
	}

}