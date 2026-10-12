#include <UI/Panels/ScenePanel.h>

#include <RZE.h>
#include <UI/Framework/EditorContext.h>
#include <UI/Framework/PanelRegistry.h>

#include <Commands/SceneCommands.h>

#include <Game/World/GameObject/GameObject.h>
#include <Game/World/GameObjectComponents/EditorCameraComponent.h>
#include <Game/World/GameObjectComponents/TransformComponent.h>

#include <Utils/DebugUtils/Debug.h>

#include <ImGui/imgui.h>

namespace
{
	constexpr size_t k_maxGameObjectNameSize = 64;

	// The dragged object's GameObjectID; names aren't unique
	constexpr char k_gameObjectDragPayload[] = "GameObjectID";

	GameObjectPtr FindGameObjectByID(GameObjectID id)
	{
		GameObjectPtr found;
		RZE().GetActiveScene().ForEachGameObject(
			Functor<void, GameObjectPtr>([&found, id](GameObjectPtr gameObject)
			{
				if (gameObject->GetID() == id)
				{
					found = gameObject;
				}
			}));
		return found;
	}
}

namespace Editor
{
	REGISTER_EDITOR_PANEL(ScenePanel);

	PanelDesc ScenePanel::Describe()
	{
		PanelDesc desc;
		desc.Name = "Scene";
		desc.MenuPath = "View/Windows/Scene";
		desc.DefaultDock = EPanelDock::Left;
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
						context.GetCommands().Push<CreateGameObjectCommand>(name);
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

		// The item code below runs whether or not the node is open, so a collapsed parent can be clicked, dragged and
		// right-clicked too; only its children and TreePop depend on it being open
		const bool isOpen = ImGui::TreeNodeEx(gameObject->GetName().c_str(), nodeFlags);

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
			const GameObjectID draggedID = gameObject->GetID();
			ImGui::SetDragDropPayload(k_gameObjectDragPayload, &draggedID, sizeof(draggedID));
			ImGui::TextUnformatted(gameObject->GetName().c_str());

			ImGui::EndDragDropSource();
		}

		if (ImGui::BeginDragDropTarget())
		{
			const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(k_gameObjectDragPayload);
			if (payload != nullptr)
			{
				const GameObjectID draggedID = *static_cast<const GameObjectID*>(payload->Data);
				context.GetCommands().Push<AttachToParentCommand>(FindGameObjectByID(draggedID), gameObject);
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
						context.GetCommands().Push<RenameGameObjectCommand>(gameObject, name);
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
							context.GetCommands().Push<AddComponentCommand>(gameObject, pair.first);
						}
					}

					ImGui::EndMenu();
				}

				if (!gameObject->IsRoot())
				{
					if (ImGui::MenuItem("DetachFromParent"))
					{
						context.GetCommands().Push<DetachFromParentCommand>(gameObject);
					}
				}

				if (ImGui::MenuItem("Delete"))
				{
					context.GetCommands().Push<DeleteGameObjectCommand>(gameObject);
				}

				ImGui::EndPopup();
			}
		}

		if (isOpen)
		{
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
