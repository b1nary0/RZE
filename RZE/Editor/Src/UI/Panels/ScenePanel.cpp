#include <UI/Panels/ScenePanel.h>

#include <EditorApp.h>

#include <Utils/DebugUtils/Debug.h>

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
			const std::vector<GameScene::SceneEntryTemp>& entities = RZE_Application::RZE().GetActiveScene().GetSceneEntries();
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

				if (HasSelectedEntity() && GetSelectedEntityID() == entry.ID)
				{
					if (ImGui::BeginPopupContextItem(entry.Name.c_str()))
					{
						ImGui::Text(entry.Name.c_str());
						ImGui::Separator();

						if (ImGui::BeginMenu("Add Component"))
						{
							const std::vector<std::string>& componentTypeNames = RZE_Application::RZE().GetActiveScene().GetEntityHandler().GetAllComponentNames();
							for (const std::string& componentTypeName : componentTypeNames)
							{
								if (ImGui::MenuItem(componentTypeName.c_str()))
								{
								}
							}

							ImGui::EndMenu();
						}

						if (ImGui::MenuItem("Delete"))
						{
							RZE_Application::RZE().GetActiveScene().DestroyEntity(mSelectedItem->EntityID);
						}

						ImGui::EndPopup();
					}
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

	bool ScenePanel::HasSelectedEntity()
	{
		return mSelectedItem != nullptr;
	}

	U32 ScenePanel::GetSelectedEntityID()
	{
		AssertNotNull(mSelectedItem);
		return mSelectedItem->EntityID;
	}

}