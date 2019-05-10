#include <UI/ScenePanel.h>

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
				if (ImGui::TreeNode(entry.Name.c_str()))
				{
					Apollo::EntityHandler::ComponentNameIDMap componentNames;
					RZE_Application::RZE().GetActiveScene().GetEntityHandler().GetComponentNames(entry.ID, componentNames);
					for (auto& pair : componentNames)
					{
						if (ImGui::Selectable(pair.second.c_str()))
						{

						}
					}
					ImGui::TreePop();
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