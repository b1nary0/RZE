#include <UI/Panels/ScenePanel.h>

#include <EditorApp.h>

#include <Game/World/GameObject/GameObject.h>
#include <Game/World/GameObjectComponents/EditorCameraComponent.h>

#include <Utils/DebugUtils/Debug.h>

#include <ImGui/imgui.h>

#include <Game/World/GameObjectComponents/TransformComponent.h>
#include "Game/World/GameObject/EditorGameComponentInfoCache.h"

namespace
{
	constexpr size_t k_maxGameObjectNameSize = 64;
}

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

	bool showGameObjectNameBoxForCreate = false;
	void ScenePanel::Display()
	{
		if (ImGui::Begin("Scene", &bEnabled))
		{
			if (!HasSelectedGameObject())
			{
				if (ImGui::BeginPopupContextWindow("SceneMenu"))
				{
					if (ImGui::MenuItem("Create GameObject"))
					{
						showGameObjectNameBoxForCreate = true;
					}
					
					ImGui::EndMenu();
				}

				if (showGameObjectNameBoxForCreate)
				{
					ImGuiWindowFlags windowFlags = 
						ImGuiWindowFlags_AlwaysAutoResize
					| ImGuiWindowFlags_NoResize
					| ImGuiWindowFlags_NoMove
					| ImGuiWindowFlags_NoCollapse;
					// @TODO Figure out why GetCursorScreenPos() doesnt give the actual position (it gets drawn at the top)
					ImGui::SetNextWindowPos(ImGui::GetCursorScreenPos());
					if (ImGui::Begin("GameObjectNameCreate", &showGameObjectNameBoxForCreate, windowFlags))
					{
						char name[k_maxGameObjectNameSize] = { 0 };
						if (ImGui::InputText("GameObject Name", name, k_maxGameObjectNameSize, ImGuiInputTextFlags_EnterReturnsTrue))
						{
							RZE().GetActiveScene().AddGameObject(name);
							showGameObjectNameBoxForCreate = false;
						}
					}
					ImGui::End();
				}
			}
			
			RZE().GetActiveScene().ForEachGameObject(
				Functor<void, GameObjectPtr>([this](GameObjectPtr gameObject)
				{
					if (ImGui::IsAnyMouseDown() && (ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered()) && HasSelectedGameObject())
					{
						m_selectedItem.reset();
					}

					// @NOTE Don't show the EditorComponent. Settings for this should go through
					// editor preferences UI
					GameObjectComponentPtr<EditorCameraComponent> cameraComponent = gameObject->GetComponent<EditorCameraComponent>();
					if (cameraComponent != nullptr)
					{
						return;
					}

					bool bSelectedCurrent = HasSelectedGameObject() && GetSelectedGameObject() == gameObject;
					if (ImGui::Selectable(gameObject->GetName().c_str(), &bSelectedCurrent, ImGuiSelectableFlags_AllowDoubleClick))
					{
						if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && HasSelectedGameObject())
						{
							GameObjectPtr gameObject = GetSelectedGameObject();
							GameObjectComponentPtr<TransformComponent> transformComponent = gameObject->GetTransformComponent();

							GameObjectPtr camera = RZE().GetActiveScene().FindGameObjectByName("EditorCam");
							GameObjectComponentPtr<EditorCameraComponent> editorCam = camera->GetComponent<EditorCameraComponent>();
							AssertNotNull(editorCam);
							GameObjectComponentPtr<TransformComponent> cameraTransform = camera->GetTransformComponent();

							editorCam->SetForward((transformComponent->GetPosition() - cameraTransform->GetPosition()).Normalized());
						}
						else 
						{
							if (!HasSelectedGameObject() || (HasSelectedGameObject() && GetSelectedGameObject() != gameObject))
							{
								SelectedItem* newItem = new SelectedItem();
								newItem->m_gameObject = gameObject;
								newItem->m_isDirty = true;

								m_selectedItem.reset(newItem);
							}
						}
					}

					if (bSelectedCurrent)
					{
						if (ImGui::BeginPopupContextItem(GetSelectedGameObject()->GetName().c_str()))
						{
							ImGui::Text(GetSelectedGameObject()->GetName().c_str());
							if (ImGui::BeginPopupContextItem("Rename"))
							{
								char name[k_maxGameObjectNameSize];
                                memcpy( name, GetSelectedGameObject()->GetName().c_str(), k_maxGameObjectNameSize );
                                if ( ImGui::InputText( "##gameObject_rename", name, k_maxGameObjectNameSize, ImGuiInputTextFlags_EnterReturnsTrue ) )
                                {
									GetSelectedGameObject()->SetName(name);
								}
								ImGui::EndPopup();
							}
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
								if (gameObject == GetSelectedGameObject())
								{
									ResetSelectedGameObject();
								}

								RZE().GetActiveScene().RemoveGameObject(gameObject);
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

				// Cache the component pointers if we've modified our components or we've changed the focused GameObject.
				if (m_selectedItem->m_isDirty || componentList.size() != m_components.size())
				{
					m_components.reserve(componentList.size());
					m_components.clear();

					const EditorComponentCache::ComponentInfoMap& componentData = EditorComponentCache::GetAllComponentsInfo();

					for (auto& component : componentList)
					{
						m_components.push_back( component );
					}

					if ( m_components.size() > 1 )
					{
 						std::sort( m_components.begin(), m_components.end(), [&componentData](const GameObjectComponentBase* a, const GameObjectComponentBase* b) {
 							int32_t first = EditorComponentCache::kDefaultSortingOrder;
							int32_t second = EditorComponentCache::kDefaultSortingOrder;
 							bool foundFirst = componentData.find(a->m_id) != componentData.end();
 							if (foundFirst)
 							{
								first = componentData.at(a->m_id).Order;
 							}
							
 							bool foundSecond = componentData.find(b->m_id) != componentData.end();
 							if (foundSecond)
 							{
								second = componentData.at(b->m_id).Order;
 							}
							
 							return first < second;
 						} );
					}

					m_selectedItem->m_isDirty = false;
				}

				for (auto& component : m_components)
				{
					if ( component )
					{
						ImGui::TextColored( ImVec4( 0.65f, 0.65f, 1.0f, 1.0f ), "[ %s ]", component->GetName().c_str() );
						ImGui::Separator();
						component->OnEditorInspect();
						ImGui::Separator();
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

	bool ScenePanel::HasSelectedGameObject()
	{
		return m_selectedItem != nullptr;
	}

	GameObjectPtr ScenePanel::GetSelectedGameObject()
	{
		AssertNotNull(m_selectedItem);
		return m_selectedItem->m_gameObject;
	}

	void ScenePanel::ResetSelectedGameObject()
	{
		m_selectedItem.reset();
	}
}
