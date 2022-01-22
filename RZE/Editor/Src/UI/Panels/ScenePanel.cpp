#include <UI/Panels/ScenePanel.h>

#include <EditorApp.h>

#include <ECS/Components/NameComponent.h>

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
		if (mSelectedItem != nullptr)
		{
			delete mSelectedItem;
			mSelectedItem = nullptr;
		}
	}

	void ScenePanel::Display()
	{
		if (ImGui::Begin("Scene", &bEnabled))
		{
			if (!HasSelectedEntity())
			{
				if (ImGui::BeginPopupContextWindow("SceneMenu"))
				{
					if (ImGui::MenuItem("Create Entity"))
					{
						//// #TODO(Do this better)
						//static int sGenericEntityCount = 0;

						//std::string newEntityStr = StringUtils::FormatString("Entity%i", sGenericEntityCount);
						//RZE_Application::RZE().GetActiveScene().CreateEntity(newEntityStr);

						//++sGenericEntityCount;
					}

					ImGui::EndMenu();
				}
			}

			//const std::vector<GameScene::SceneEntryTemp>& entities = RZE_Application::RZE().GetActiveScene().GetSceneEntries();
			//for (const GameScene::SceneEntryTemp& entry : entities)
			{
				if (ImGui::IsAnyMouseDown() && (ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered()) && HasSelectedEntity())
				{
					delete mSelectedItem;
					mSelectedItem = nullptr;
				}

				bool bSelectedCurrent = mSelectedItem != nullptr /*&& mSelectedItem->EntityID == entry.ID*/;
				/*NameComponent* const nameComponent = RZE_Application::RZE().GetActiveScene().GetEntityHandler().GetComponent<NameComponent>(entry.ID);
				const std::string& entityName = nameComponent->Name;
				AssertNotNull(nameComponent);*/
				if (ImGui::Selectable("UNIMPLEMENTED ENTITY NAME", &bSelectedCurrent))
				{
					if (mSelectedItem != nullptr /*&& mSelectedItem->EntityID != entry.ID*/)
					{
						delete mSelectedItem;
						mSelectedItem = nullptr;
					}

					/*mSelectedItem = new EntityItem();
					mSelectedItem->EntityID = entry.ID;*/
				}

				if (HasSelectedEntity() /*&& GetSelectedEntityID() == entry.ID*/)
				{
					if (ImGui::BeginPopupContextItem("UNIMPLEMENTED ENTITY NAME"))
					{
						ImGui::Text("UNIMPLEMENTED ENTITY NAME");
						ImGui::Separator();

						if (ImGui::BeginMenu("Add Component"))
						{
							/*const Apollo::EntityHandler::ComponentNameIDMap& componentTypeNames = RZE_Application::RZE().GetActiveScene().GetEntityHandler().GetAllComponentTypes();
							for (auto& pair : componentTypeNames)
							{
								if (ImGui::MenuItem(pair.second.c_str()))
								{
									Apollo::ComponentID componentID = pair.first;
									RZE_Application::RZE().GetActiveScene().GetEntityHandler().AddComponentByID(mSelectedItem->EntityID, componentID);
								}
							}*/

							ImGui::EndMenu();
						}

						if (ImGui::MenuItem("Delete"))
						{
							//RZE_Application::RZE().GetActiveScene().DestroyEntity(mSelectedItem->EntityID);
						}

						ImGui::EndPopup();
					}
				}
			}
		}
		ImGui::End();

		if (ImGui::Begin("Component View"))
		{
			if (HasSelectedEntity())
			{
				//Apollo::EntityHandler::ComponentList entityComponents;
				//RZE_Application::RZE().GetActiveScene().GetEntityHandler().GetAllComponents(mSelectedItem->EntityID, entityComponents);

				//for (Apollo::ComponentBase* component : entityComponents)
				{
					EditorApp* editorApp = static_cast<EditorApp*>(&RZE_Application::RZE().GetApplication());

					ImGui::TextColored(ImVec4(0.65f, 0.65f, 1.0f, 1.0f), "[ %s ]", "UNIMPLEMENTED COMPONENT NAME");
					ImGui::Separator();
					//component->OnEditorInspect(mSelectedItem->EntityID);
					ImGui::Separator();
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