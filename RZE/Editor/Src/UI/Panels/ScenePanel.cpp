#include <UI/Panels/ScenePanel.h>

#include <RZE.h>
#include <UI/Framework/EditorContext.h>

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
	{
	}

	ScenePanel::~ScenePanel()
	{
	}

	bool showGameObjectNameBoxForCreate = false;
	void ScenePanel::Display(EditorContext& context)
	{
		EditorSelection& selection = context.GetSelection();

		if (ImGui::Begin("Scene", &bEnabled))
		{
			if (selection.IsEmpty())
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

						if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && ImGui::IsKeyPressed(ImGuiKey_Escape, false))
						{
							showGameObjectNameBoxForCreate = false;
						}
					}
					ImGui::End();
				}
			}
			
			if (ImGui::IsAnyMouseDown() && (ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered()) && !selection.IsEmpty())
			{
				selection.Clear();
			}

			RZE().GetActiveScene().ForEachGameObject(
				Functor<void, GameObjectPtr>([this, &context](GameObjectPtr gameObject)
				{
					GameObjectPtr selectedObject;
					if (gameObject->IsRoot())
					{
						// @NOTE Don't show the EditorComponent. Settings for this should go through
						// editor preferences UI
						GameObjectComponentPtr<EditorCameraComponent> cameraComponent = gameObject->GetComponent<EditorCameraComponent>();
						if (cameraComponent != nullptr)
						{
							return;
						}

						DisplayObject(context, gameObject);
					}
				}));
		}
		ImGui::End();

		if (ImGui::Begin("Component View"))
		{
			if (!selection.IsEmpty())
			{
				GameObjectPtr primary = selection.GetPrimary();
				const GameObject::ComponentList& componentList = primary->GetComponents();

				// Cache the component pointers if we've modified our components or we've changed the focused GameObject.
				if (m_inspectedObject != primary || componentList.size() != m_components.size())
				{
					m_components.reserve(componentList.size());
					m_components.assign(componentList.begin(), componentList.end());

					if (m_components.size() > 1)
					{
						const EditorComponentCache::ComponentInfoMap& componentData = EditorComponentCache::GetAllComponentsInfo();

 						std::sort(m_components.begin(), m_components.end(), [&componentData](const GameObjectComponentBase* a, const GameObjectComponentBase* b) {
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
 						});
					}

					m_inspectedObject = primary;
				}

				for (auto& component : m_components)
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

	void ScenePanel::DisplayObject(EditorContext& context, GameObjectPtr gameObject)
	{
		EditorSelection& selection = context.GetSelection();

		int nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow;
		if (selection.IsSelected(gameObject))
		{
			nodeFlags |= ImGuiTreeNodeFlags_Selected;
		}

		if (!gameObject->HasChildren())
		{
			nodeFlags |= ImGuiTreeNodeFlags_Leaf;
		}

		if (ImGui::TreeNodeEx(gameObject->GetName().c_str(), nodeFlags))
		{
			if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
			{
				if (ImGui::GetIO().KeyCtrl)
				{
					selection.Toggle(gameObject);
				}
				else
				{
					selection.Select(gameObject);
				}
			}

			if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
			{
				Vector3D boundsMin;
				Vector3D boundsMax;
				if (!gameObject->GetHierarchyBounds(boundsMin, boundsMax))
				{
					// Nothing to measure (light, empty, ...); frame its position and let the camera's minimum radius pick a distance
					boundsMin = boundsMax = gameObject->GetTransformComponent()->GetWorldMatrix().GetPosition();
				}

				// Absent while a scene is loading
				GameObjectComponentPtr<EditorCameraComponent> editorCam = context.GetEditorCameraComponent();
				if (editorCam != nullptr)
				{
					editorCam->FocusOn(boundsMin, boundsMax);
				}
			}

			if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
			{
				ImGui::SetDragDropPayload("DraggedObjectName", gameObject->GetName().c_str(), gameObject->GetName().size());

				ImGui::EndDragDropSource();
			}

			if (ImGui::BeginDragDropTarget())
			{
				const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DraggedObjectName");
				if (payload != nullptr)
				{
					const char* draggedObjectName = static_cast<const char*>(payload->Data);
					GameObjectPtr draggedObject = RZE().GetActiveScene().FindGameObjectByName(draggedObjectName);
					AssertNotNull(draggedObject);
					if (!draggedObject->IsRoot())
					{
						draggedObject->DetachFromParent();
					}

					draggedObject->AttachTo(gameObject);
				}
				ImGui::EndDragDropTarget();
			}

			if (!selection.IsEmpty())
			{
				GameObjectPtr gameObject = selection.GetPrimary();
				if (ImGui::BeginPopupContextItem(gameObject->GetName().c_str()))
				{
					ImGui::Text(gameObject->GetName().c_str());
					if (ImGui::BeginPopupContextItem("Rename"))
					{
						char name[k_maxGameObjectNameSize];
						memcpy(name, gameObject->GetName().c_str(), k_maxGameObjectNameSize);
						if (ImGui::InputText("##gameObject_rename", name, k_maxGameObjectNameSize, ImGuiInputTextFlags_EnterReturnsTrue))
						{
							gameObject->SetName(name);
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
								gameObject->AddComponentByID(componentID);
							}
						}

						ImGui::EndMenu();
					}

					if (!gameObject->IsRoot())
					{
						if (ImGui::MenuItem("DetachFromParent"))
						{
							gameObject->DetachFromParent();
						}
					}

					if (ImGui::MenuItem("Delete"))
					{
						selection.Clear();

						RZE().GetActiveScene().RemoveGameObject(gameObject);
					}

					ImGui::EndPopup();
				}
			}

			if (gameObject->HasChildren())
			{
				for (int childIdx = 0; childIdx < gameObject->NumChildren(); ++childIdx)
				{
					GameObjectPtr child = gameObject->GetChildAtIndex(childIdx);
					DisplayObject(context, child);
				}
			}

			ImGui::TreePop();
		}
	}
}
