#include <UI/Panels/ObjectBrowserPanel.h>

#include <RZE.h>
#include <UI/Framework/EditorContext.h>
#include <UI/Framework/EditorSelection.h>
#include <UI/Framework/EditorStyle.h>
#include <UI/Framework/EditorUI.h>
#include <UI/Framework/PanelRegistry.h>

#include <Commands/SceneCommands.h>

#include <Game/World/GameObject/GameObject.h>
#include <Game/World/GameObjectComponents/EditorCameraComponent.h>
#include <Game/World/GameObjectComponents/TransformComponent.h>

#include <Utils/DebugUtils/Debug.h>
#include <Utils/StringUtils.h>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h> // ScrollToRect

#include <Optick/optick.h>

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
	REGISTER_EDITOR_PANEL(ObjectBrowserPanel);

	struct ObjectBrowserPanel::ObjectRename
	{
		GameObjectPtr Object;
		char Name[k_maxGameObjectNameSize] = {};
		// Its place in the tree as of the last frame; -1 until the tree has been walked with it. The clipper draws that
		// row wherever it's scrolled to, so the text box keeps the keyboard.
		int RowIndex = -1;
		// The text box takes the keyboard on its next draw, e.g. when it appears
		bool FocusBox = true;
		// Its row is scrolled into view on its next draw: a new object's is at the bottom
		bool ScrollToRow = true;
	};

	ObjectBrowserPanel::ObjectBrowserPanel() = default;
	ObjectBrowserPanel::~ObjectBrowserPanel() = default;

	PanelDesc ObjectBrowserPanel::Describe()
	{
		PanelDesc desc;
		desc.Name = "Object Browser";
		desc.MenuPath = "View/Windows/Object Browser";
		desc.DefaultDock = EPanelDock::RightTop;
		// The toolbar sits in the menu bar, so it isn't scrolled away with the rows. A scene that fills the panel leaves
		// no empty space to right-click for Create GameObject.
		desc.WindowFlags = ImGuiWindowFlags_MenuBar;
		// The same space around its controls as the Log panel's and the scene view's toolbars
		desc.MenuBarPadding = 6.0f;
		desc.MenuBarSeparator = true;
		return desc;
	}

	void ObjectBrowserPanel::Initialize(EditorContext& context)
	{
		// The object made by Create GameObject is named in place. The command runs at the end of the UI frame, after
		// this panel has drawn, so the object is only known here.
		m_gameObjectCreated = context.GetEvents().Subscribe<GameObjectCreatedEvent>([this](const GameObjectCreatedEvent& event)
			{
				BeginRename(event.GameObject);
			});
	}

	void ObjectBrowserPanel::DrawMenuBar(EditorContext& context)
	{
		// At the bar's right edge, inset as far as its contents are on the left. Not the content region's: that stops at
		// the scrollbar, so the button would move as the scene grows long enough to scroll.
		const char* const addLabel = "Add GameObject";
		const ImGuiWindow* const window = ImGui::GetCurrentWindow();
		const float right = window->MenuBarRect().Max.x - window->DC.MenuBarOffset.x;
		const float buttonWidth = ImGui::CalcTextSize(addLabel).x + ImGui::GetStyle().FramePadding.x * 2.0f;
		ImGui::SetCursorScreenPos(ImVec2(right - buttonWidth, ImGui::GetCursorScreenPos().y));

		// The new object is named in place, as from the menu (GameObjectCreatedEvent)
		if (ImGui::Button(addLabel))
		{
			context.GetCommands().Push<CreateGameObjectCommand>();
		}
		EditorUI::ItemTooltip("Adds an empty GameObject at the top level of the scene and names it");
	}

	void ObjectBrowserPanel::OnSceneReset()
	{
		m_rename.reset();
		m_rangeAnchor = nullptr;
	}

	void ObjectBrowserPanel::Draw(EditorContext& context)
	{
		// Its own scope, as it grows with the scene: a profile can take it off the frame to measure the scene itself
		OPTICK_EVENT();

		EditorSelection& selection = context.GetSelection();

		// Only over empty space: a row opens its own menu (DisplayObject). Creating doesn't use the selection, so this
		// opens whatever is selected
		if (ImGui::BeginPopupContextWindow("ObjectBrowserMenu", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
		{
			if (ImGui::MenuItem("Create GameObject"))
			{
				context.GetCommands().Push<CreateGameObjectCommand>();
			}

			ImGui::EndPopup();
		}

		if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows) && !ImGui::IsAnyItemActive() && ImGui::IsKeyPressed(ImGuiKey_F2, false)
			&& selection.GetPrimary() != nullptr)
		{
			BeginRename(selection.GetPrimary());
		}

		// Only under the rows: not on the toolbar beside its button, or on a floating panel's title bar
		const bool isOverEmptySpace = ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered()
			&& ImGui::GetCurrentWindow()->InnerRect.Contains(ImGui::GetMousePos());
		if (ImGui::IsAnyMouseDown() && isOverEmptySpace && !selection.IsEmpty())
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
		if (m_rename != nullptr && m_rename->RowIndex >= 0)
		{
			clipper.IncludeItemByIndex(m_rename->RowIndex);
		}
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

		if (m_rename != nullptr)
		{
			const auto renamedRow = std::find(m_visibleRows.begin(), m_visibleRows.end(), m_rename->Object);
			if (renamedRow == m_visibleRows.end())
			{
				m_rename.reset();
			}
			else
			{
				m_rename->RowIndex = static_cast<int>(std::distance(m_visibleRows.begin(), renamedRow));
			}
		}

		if (m_rangeClickTarget != nullptr)
		{
			SelectRange(selection, m_rangeClickTarget, m_isRangeAdded);
		}
	}

	bool ObjectBrowserPanel::DisplayObject(EditorContext& context, GameObjectPtr gameObject)
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

		// The rename's text box goes over the row once it's drawn, in place of the name
		const bool isRenaming = m_rename != nullptr && m_rename->Object == gameObject;

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(style.FramePadding.x, k_rowPaddingY));
		// Where the node draws its name
		const float labelX = ImGui::GetCursorScreenPos().x + ImGui::GetTreeNodeToLabelSpacing();
		ImGui::SetNextItemStorageID(GetOpenStateID(gameObject));
		const bool isOpen = EditorUI::TreeNode("##GameObject", nodeFlags, "%s", isRenaming ? "" : gameObject->GetName().c_str());
		ImGui::PopStyleVar();
		const ImVec2 rowMin = ImGui::GetItemRectMin();
		const ImVec2 rowMax = ImGui::GetItemRectMax();

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
			ImGui::Separator();

			if (ImGui::MenuItem("Rename", "F2"))
			{
				BeginRename(gameObject);
			}

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

		if (isRenaming)
		{
			DrawRenameBox(context, rowMin, rowMax, labelX);
		}

		ImGui::PopID();
		// A leaf "opens" too, having nothing to collapse
		return isOpen && gameObject->HasChildren();
	}

	void ObjectBrowserPanel::BeginRename(GameObjectPtr gameObject)
	{
		m_rename = std::make_unique<ObjectRename>();
		m_rename->Object = gameObject;
		StringUtils::CopyTruncated(m_rename->Name, gameObject->GetName());
	}

	void ObjectBrowserPanel::DrawRenameBox(EditorContext& context, const ImVec2& rowMin, const ImVec2& rowMax, float labelX)
	{
		ObjectRename& rename = *m_rename;
		if (rename.ScrollToRow)
		{
			// Only as far as it takes to show the whole row; nothing if it's in view
			ImGui::ScrollToRect(ImGui::GetCurrentWindow(), ImRect(rowMin, rowMax), ImGuiScrollFlags_KeepVisibleEdgeY);
			rename.ScrollToRow = false;
		}

		// Small enough padding that the box sits where the name was, centred in the row
		const ImVec2 padding(3.0f, 1.0f);
		const float boxHeight = ImGui::GetFontSize() + padding.y * 2.0f;
		ImGui::SetCursorScreenPos(ImVec2(labelX - padding.x, rowMin.y + (rowMax.y - rowMin.y - boxHeight) * 0.5f));
		ImGui::SetNextItemWidth(rowMax.x - labelX - ImGui::GetStyle().FramePadding.x);
		if (rename.FocusBox)
		{
			ImGui::SetKeyboardFocusHere();
			rename.FocusBox = false;
		}

		const bool isNameEmpty = rename.Name[0] == '\0';
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, padding);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
		ImGui::PushStyleColor(ImGuiCol_Border, isNameEmpty ? EditorStyle::k_errorColour : EditorStyle::k_accentColour);
		const bool isEntered = ImGui::InputText("##Rename", rename.Name, sizeof(rename.Name), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
		ImGui::PopStyleColor();
		ImGui::PopStyleVar(2);

		if (isNameEmpty && ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("A GameObject needs a name");
		}

		if (isEntered)
		{
			if (rename.Name[0] == '\0')
			{
				// Enter let go of the box; it keeps the keyboard until there's a name or the rename is cancelled
				rename.FocusBox = true;
				return;
			}

			// The command does nothing if the name is the same
			context.GetCommands().Push<RenameGameObjectCommand>(rename.Object, std::string(rename.Name));
			m_rename.reset();
		}
		else if (ImGui::IsItemDeactivated())
		{
			// Escape, or a click elsewhere
			m_rename.reset();
		}
	}

	void ObjectBrowserPanel::SelectRange(EditorSelection& selection, GameObjectPtr target, bool addToSelection)
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
