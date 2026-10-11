#include <UI/Panels/ScenePanel.h>

#include <RZE.h>
#include <UI/Framework/EditorContext.h>
#include <UI/Framework/PanelRegistry.h>

#include <Game/World/GameObject/GameObject.h>
#include <Game/World/GameObjectComponents/EditorCameraComponent.h>
#include <Game/World/GameObjectComponents/TransformComponent.h>

#include <Utils/DebugUtils/Debug.h>

#include <ImGui/imgui.h>

namespace
{
	constexpr size_t k_maxGameObjectNameSize = 64;
}

namespace Editor
{
	REGISTER_EDITOR_PANEL(ScenePanel);

	PanelDesc ScenePanel::Describe()
	{
		PanelDesc desc;
		desc.Name = "Scene";
		desc.MenuPath = "Windows/Scene";
		return desc;
	}

	void ScenePanel::OnSceneReset()
	{
		m_isCreatingGameObject = false;
	}

	void ScenePanel::Draw(EditorContext& context)
	{
		EditorSelection& selection = context.GetSelection();

		if (selection.IsEmpty())
		{
			if (ImGui::BeginPopupContextWindow("SceneMenu"))
			{
				if (ImGui::MenuItem("Create GameObject"))
				{
					m_isCreatingGameObject = true;
				}

				ImGui::EndPopup();
			}

			if (m_isCreatingGameObject)
			{
				ImGuiWindowFlags windowFlags =
					ImGuiWindowFlags_AlwaysAutoResize
				| ImGuiWindowFlags_NoResize
				| ImGuiWindowFlags_NoMove
				| ImGuiWindowFlags_NoCollapse;
				// @TODO Figure out why GetCursorScreenPos() doesnt give the actual position (it gets drawn at the top)
				ImGui::SetNextWindowPos(ImGui::GetCursorScreenPos());
				if (ImGui::Begin("GameObjectNameCreate", &m_isCreatingGameObject, windowFlags))
				{
					char name[k_maxGameObjectNameSize] = { 0 };
					if (ImGui::InputText("GameObject Name", name, k_maxGameObjectNameSize, ImGuiInputTextFlags_EnterReturnsTrue))
					{
						RZE().GetActiveScene().AddGameObject(name);
						m_isCreatingGameObject = false;
					}

					if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && ImGui::IsKeyPressed(ImGuiKey_Escape, false))
					{
						m_isCreatingGameObject = false;
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
