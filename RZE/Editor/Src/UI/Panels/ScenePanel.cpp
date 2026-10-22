#include <UI/Panels/ScenePanel.h>

#include <RZE.h>
#include <UI/Framework/EditorContext.h>
#include <UI/Framework/EditorSelection.h>
#include <UI/Framework/EditorUI.h>
#include <UI/Framework/PanelRegistry.h>

#include <Commands/SceneCommands.h>

#include <Game/World/GameObject/GameObject.h>
#include <Game/World/GameObjectComponents/EditorCameraComponent.h>
#include <Game/World/GameObjectComponents/TransformComponent.h>

#include <Utils/DebugUtils/Debug.h>

#include <ImGui/imgui.h>

#include <algorithm>
#include <iterator>

namespace
{
	constexpr size_t k_maxGameObjectNameSize = 64;

	// Space above and below each object's name, inside its row so a click there still selects it
	constexpr float k_rowPaddingY = 4.0f;

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
		desc.DefaultDock = EPanelDock::RightTop;
		return desc;
	}

	void ScenePanel::OnSceneReset()
	{
		m_isCreatingGameObject = false;
		m_rangeAnchor = nullptr;
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

		// A row per GameObject, every other one shaded so they're easier to tell apart
		if (!ImGui::BeginTable("##Hierarchy", 1, ImGuiTableFlags_RowBg))
		{
			return;
		}

		m_visibleRows.clear();
		m_rangeClickTarget = nullptr;

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

		ImGui::EndTable();

		if (m_rangeClickTarget != nullptr)
		{
			SelectRange(selection, m_rangeClickTarget, m_isRangeAdded);
		}
	}

	void ScenePanel::DisplayObject(EditorContext& context, GameObjectPtr gameObject)
	{
		EditorSelection& selection = context.GetSelection();

		// Clicked anywhere across its row, not just on its name
		int nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_SpanFullWidth;
		if (selection.IsSelected(gameObject))
		{
			nodeFlags |= ImGuiTreeNodeFlags_Selected;
		}

		if (!gameObject->HasChildren())
		{
			nodeFlags |= ImGuiTreeNodeFlags_Leaf;
		}

		// Names aren't unique, so the row's IDs come from the GUID; the label is only displayed
		ImGui::PushID(reinterpret_cast<const void*>(static_cast<uintptr_t>(gameObject->GetGUID())));
		// The row has no padding of its own: the tree node pads itself to fill it, so a click between two objects lands
		// on one of them instead of on the panel, which would clear the selection
		const ImGuiStyle& style = ImGui::GetStyle();
		ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(style.CellPadding.x, 0.0f));
		ImGui::TableNextRow();
		ImGui::PopStyleVar();
		ImGui::TableSetColumnIndex(0);

		// The item code below runs whether or not the node is open, so a collapsed parent can be clicked, dragged and
		// right-clicked too; only its children and TreePop depend on it being open
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(style.FramePadding.x, k_rowPaddingY));
		const bool isOpen = EditorUI::TreeNode("##GameObject", nodeFlags, "%s", gameObject->GetName().c_str());
		ImGui::PopStyleVar();
		m_visibleRows.push_back(gameObject);

		if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
		{
			const ImGuiIO& io = ImGui::GetIO();
			if (io.KeyShift)
			{
				// Ctrl+Shift adds the range to what's already selected
				m_rangeClickTarget = gameObject;
				m_isRangeAdded = io.KeyCtrl;
			}
			else
			{
				if (io.KeyCtrl)
				{
					selection.Toggle(gameObject);
				}
				else
				{
					selection.Select(gameObject);
				}
				m_rangeAnchor = gameObject;
			}
		}

		if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
		{
			BoundingBox bounds = gameObject->GetHierarchyBounds();
			if (bounds.IsEmpty())
			{
				// Nothing to measure (light, empty, ...); frame its position and let the camera's minimum radius pick a distance
				bounds.Grow(gameObject->GetTransformComponent()->GetWorldMatrix().GetPosition());
			}

			// Absent while a scene is loading
			GameObjectComponentPtr<EditorCameraComponent> editorCam = context.GetEditorCameraComponent();
			if (editorCam != nullptr)
			{
				editorCam->FocusOn(bounds);
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
			if (ImGui::BeginPopupContextItem("GameObjectMenu"))
			{
				ImGui::TextUnformatted(gameObject->GetName().c_str());
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

		ImGui::PopID();
	}

	void ScenePanel::SelectRange(EditorSelection& selection, GameObjectPtr target, bool addToSelection)
	{
		// The anchor is stale once the selection is changed elsewhere (e.g. picked in the scene view); range from the
		// primary then instead
		const GameObjectPtr anchor = selection.IsSelected(m_rangeAnchor) ? m_rangeAnchor : selection.GetPrimary();
		const auto anchorIt = std::find(m_visibleRows.begin(), m_visibleRows.end(), anchor);
		const auto targetIt = std::find(m_visibleRows.begin(), m_visibleRows.end(), target);
		AssertExpr(targetIt != m_visibleRows.end());

		if (anchor == nullptr || anchorIt == m_visibleRows.end())
		{
			// Nothing to range from, or it's hidden under a collapsed parent: a plain click
			selection.Select(target);
			m_rangeAnchor = target;
			return;
		}

		// From the anchor towards the target, so the target is last and becomes the primary
		std::vector<GameObjectPtr> range;
		if (anchorIt <= targetIt)
		{
			range.assign(anchorIt, targetIt + 1);
		}
		else
		{
			range.assign(std::make_reverse_iterator(anchorIt + 1), std::make_reverse_iterator(targetIt));
		}

		if (addToSelection)
		{
			// Moved to the end in range order, so the target still becomes the primary
			std::vector<GameObjectPtr> combined;
			for (const GameObjectPtr& gameObject : selection.GetAll())
			{
				if (std::find(range.begin(), range.end(), gameObject) == range.end())
				{
					combined.push_back(gameObject);
				}
			}
			combined.insert(combined.end(), range.begin(), range.end());
			range = std::move(combined);
		}

		selection.SetSelection(range);
		// The anchor stays put, so another Shift+click re-ranges from the same row
		m_rangeAnchor = anchor;
	}
}
