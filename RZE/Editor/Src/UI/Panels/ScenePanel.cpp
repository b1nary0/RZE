#include <UI/Panels/ScenePanel.h>

#include <EditorApp.h>

#include <Game/World/GameObject/GameObject.h>

#include <Utils/DebugUtils/Debug.h>

#include <ImGui/imgui.h>

namespace Editor
{

	ScenePanel::ScenePanel()
		: bEnabled(false)
		, m_selectedItem(nullptr)
	{
	}

	ScenePanel::~ScenePanel()
	{
	}

	void ScenePanel::Display()
	{
		if (ImGui::Begin("Scene", &bEnabled))
		{
			if (!HasSelectedGameObject())
			{
				if (ImGui::BeginPopupContextWindow("SceneMenu"))
				{
					if (ImGui::MenuItem("Create Entity"))
					{
						//// #TODO(Do this better)
						static int sGenericEntityCount = 0;

						std::string newEntityStr = StringUtils::FormatString("Entity%i", sGenericEntityCount);
						RZE::GetActiveScene().AddGameObject(newEntityStr);

						++sGenericEntityCount;
					}

					ImGui::EndMenu();
				}
			}
			
			RZE::GetActiveScene().ForEachGameObject(
				Functor<void, std::shared_ptr<GameObject>>([this](std::shared_ptr<GameObject> gameObject)
				{
					if (ImGui::IsAnyMouseDown() && (ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered()) && HasSelectedGameObject())
					{
						m_selectedItem.reset();
					}

					bool bSelectedCurrent = HasSelectedGameObject() && GetSelectedGameObject() == gameObject;
					if (ImGui::Selectable(gameObject->GetName().c_str(), &bSelectedCurrent))
					{
						if (!HasSelectedGameObject() || (HasSelectedGameObject() && GetSelectedGameObject() != gameObject))
						{
							SelectedItem* newItem = new SelectedItem();
							newItem->m_gameObject = gameObject;

							m_selectedItem.reset(newItem);
						}
					}

					if (HasSelectedGameObject() && GetSelectedGameObject() == gameObject)
					{
						if (ImGui::BeginPopupContextItem(GetSelectedGameObject()->GetName().c_str()))
						{
							ImGui::Text(GetSelectedGameObject()->GetName().c_str());
							ImGui::Separator();

							if (ImGui::BeginMenu("Add Component"))
							{
								const GameObjectComponentRegistry::ComponentNameIDMap& componentReflectData = GameObjectComponentRegistry::GetAllComponentReflectData();
								for (auto& pair : componentReflectData)
								{
									if (ImGui::MenuItem(pair.second.c_str()))
									{
										GameObjectComponentID componentID = pair.first;
										GetSelectedGameObject()->AddComponentByID(componentID);
									}
								}

								ImGui::EndMenu();
							}

							if (ImGui::MenuItem("Delete"))
							{
								RZE::GetActiveScene().RemoveGameObject(gameObject);
							}

							ImGui::EndPopup();
						}
					}
				}));
		}
		ImGui::End();

		if (ImGui::Begin("Component View"))
		{
			if (HasSelectedGameObject())
			{
				const GameObject::ComponentList& componentList = GetSelectedGameObject()->GetComponents();
				for (auto& component : componentList)
				{
					ImGui::TextColored(ImVec4(0.65f, 0.65f, 1.0f, 1.0f), "[ %s ]", component->GetName().c_str());
					ImGui::Separator();
					component->OnEditorInspect();
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

	bool ScenePanel::HasSelectedGameObject()
	{
		return m_selectedItem != nullptr;
	}

	std::shared_ptr<GameObject> ScenePanel::GetSelectedGameObject()
	{
		AssertNotNull(m_selectedItem);
		return m_selectedItem->m_gameObject;
	}

}