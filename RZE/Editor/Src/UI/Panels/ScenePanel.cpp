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

#include <imgui/imgui.h>

#include <algorithm>
#include <climits>
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

	const void* GetRowID(const GameObjectPtr& gameObject)
	{
		return reinterpret_cast<const void*>(static_cast<uintptr_t>(gameObject->GetGUID()));
	}

	// Where a row's open state is kept: under its GUID rather than its place in the ID stack, so the state of a row
	// that's scrolled out of view, and isn't drawn, can still be read
	ImGuiID GetOpenStateID(const GameObjectPtr& gameObject)
	{
		const ObjectGUID guid = gameObject->GetGUID();
		return static_cast<ImGuiID>(guid ^ (guid >> 32));
	}

	// Steps through the rows the tree shows, top to bottom: each root, and the children of each open object, depth
	// first. It can stop and go on later, as the clipper hands out the rows to draw a range at a time.
	class RowWalk
	{
	public:
		explicit RowWalk(const std::vector<GameObjectPtr>& roots);

		bool IsDone() const { return m_levels.empty(); }
		GameObjectPtr GetCurrent() const;

		// On to the next row: the current one's first child if it's open. Each level is an ImGui tree level too,
		// pushed and popped here, so the rows in it are indented whether or not its parent's row was drawn.
		void Advance(bool isOpen);

	private:
		struct Level
		{
			// nullptr for the roots
			GameObjectPtr Parent;
			int NextIndex = 0;
		};

		int GetCount(const Level& level) const;
		// Out of every level the walk has reached the end of
		void PopFinishedLevels();

		const std::vector<GameObjectPtr>& m_roots;
		std::vector<Level> m_levels;
	};

	RowWalk::RowWalk(const std::vector<GameObjectPtr>& roots)
		: m_roots(roots)
	{
		m_levels.push_back(Level());
		PopFinishedLevels();
	}

	GameObjectPtr RowWalk::GetCurrent() const
	{
		const Level& level = m_levels.back();
		return level.Parent != nullptr ? level.Parent->GetChildAtIndex(level.NextIndex) : m_roots[level.NextIndex];
	}

	void RowWalk::Advance(bool isOpen)
	{
		const GameObjectPtr current = GetCurrent();
		++m_levels.back().NextIndex;
		if (isOpen && current->HasChildren())
		{
			ImGui::TreePush(GetRowID(current));
			m_levels.push_back({ current, 0 });
		}
		PopFinishedLevels();
	}

	int RowWalk::GetCount(const Level& level) const
	{
		return level.Parent != nullptr ? level.Parent->NumChildren() : static_cast<int>(m_roots.size());
	}

	void RowWalk::PopFinishedLevels()
	{
		while (!m_levels.empty() && m_levels.back().NextIndex >= GetCount(m_levels.back()))
		{
			if (m_levels.back().Parent != nullptr)
			{
				ImGui::TreePop();
			}
			m_levels.pop_back();
		}
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

		// Only over empty space: a row opens its own menu (DisplayObject). Creating doesn't use the selection, so this
		// opens whatever is selected
		if (ImGui::BeginPopupContextWindow("SceneMenu", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
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

		// The scene keeps no list of its roots
		m_roots.clear();
		RZE().GetActiveScene().ForEachGameObject(
			Functor<void, GameObjectPtr>([this](GameObjectPtr gameObject)
			{
				// @NOTE Don't show the EditorComponent. Settings for this should go through
				// editor preferences UI
				if (gameObject->IsRoot() && gameObject->GetComponent<EditorCameraComponent>() == nullptr)
				{
					m_roots.push_back(gameObject);
				}
			}));

		// Only the rows in view are drawn: ImGui's own clipping skips drawing the rest, but only after laying each one
		// out. The rows before them are walked to find where they start, as an open object's children are rows too.
		RowWalk walk(m_roots);
		ImGuiListClipper clipper;
		clipper.Begin(INT_MAX);
		const auto visitRow = [this, &context, &walk, &clipper]()
		{
			const GameObjectPtr gameObject = walk.GetCurrent();
			const bool isInView = clipper.UserIndex >= clipper.DisplayStart && clipper.UserIndex < clipper.DisplayEnd;
			const bool isOpen = isInView
				? DisplayObject(context, gameObject)
				: gameObject->HasChildren() && ImGui::TreeNodeGetOpen(GetOpenStateID(gameObject));
			m_visibleRows.push_back(gameObject);
			++clipper.UserIndex;
			walk.Advance(isOpen);
		};
		while (clipper.Step())
		{
			while (clipper.UserIndex < clipper.DisplayEnd && !walk.IsDone())
			{
				visitRow();
			}
		}
		// The rest too, nothing drawn: their count sizes the scrollbar, and a Shift+click range can reach them
		while (!walk.IsDone())
		{
			visitRow();
		}
		clipper.SeekCursorForItem(clipper.UserIndex);

		ImGui::EndTable();

		if (m_rangeClickTarget != nullptr)
		{
			SelectRange(selection, m_rangeClickTarget, m_isRangeAdded);
		}
	}

	bool ScenePanel::DisplayObject(EditorContext& context, GameObjectPtr gameObject)
	{
		EditorSelection& selection = context.GetSelection();

		// Clicked anywhere across its row, not just on its name. The node doesn't open a tree level of its own: the walk
		// does (RowWalk::Advance), as it must for a row that isn't drawn
		int nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_SpanFullWidth
			| ImGuiTreeNodeFlags_NoTreePushOnOpen;
		if (selection.IsSelected(gameObject))
		{
			nodeFlags |= ImGuiTreeNodeFlags_Selected;
		}

		if (!gameObject->HasChildren())
		{
			nodeFlags |= ImGuiTreeNodeFlags_Leaf;
		}

		// Names aren't unique, so the row's IDs come from the GUID; the label is only displayed
		ImGui::PushID(GetRowID(gameObject));
		// The row has no padding of its own: the tree node pads itself to fill it, so a click between two objects lands
		// on one of them instead of on the panel, which would clear the selection
		const ImGuiStyle& style = ImGui::GetStyle();
		ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(style.CellPadding.x, 0.0f));
		ImGui::TableNextRow();
		ImGui::PopStyleVar();
		ImGui::TableSetColumnIndex(0);

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(style.FramePadding.x, k_rowPaddingY));
		ImGui::SetNextItemStorageID(GetOpenStateID(gameObject));
		const bool isOpen = EditorUI::TreeNode("##GameObject", nodeFlags, "%s", gameObject->GetName().c_str());
		ImGui::PopStyleVar();

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

		// The menu acts on this row, so an unselected row is selected first and the Inspector shows what the menu will
		// change. A selected row keeps the selection, so a multi-selection isn't lost
		if (ImGui::IsItemClicked(ImGuiMouseButton_Right) && !selection.IsSelected(gameObject))
		{
			selection.Select(gameObject);
			m_rangeAnchor = gameObject;
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

		ImGui::PopID();
		// A leaf "opens" too, having nothing to collapse
		return isOpen && gameObject->HasChildren();
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
