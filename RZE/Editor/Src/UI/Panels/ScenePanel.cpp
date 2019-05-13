#include <UI/Panels/ScenePanel.h>

#include <EditorApp.h>

#include <ImGui/imgui.h>

namespace Editor
{

	ScenePanel::ScenePanel()
		: bEnabled(false)
		, mSelectedItem(nullptr)
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
					if (mSelectedItem != nullptr)
					{
						delete mSelectedItem;
					}

					mSelectedItem = new EntityItem();
					mSelectedItem->EntityID = entry.ID;
				}
			}
		}
		ImGui::End();

		if (ImGui::Begin("Component View"))
		{
			if (mSelectedItem != nullptr)
			{
				Apollo::EntityHandler::ComponentNameIDMap componentMap;
				RZE_Application::RZE().GetActiveScene().GetEntityHandler().GetComponentNames(mSelectedItem->EntityID, componentMap);

				for (auto& pair : componentMap)
				{
					if (ImGui::Selectable(pair.second.c_str()))
					{

					}
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