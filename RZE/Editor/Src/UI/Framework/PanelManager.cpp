#include <UI/Framework/PanelManager.h>

#include <UI/Framework/EditorContext.h>
#include <UI/Framework/PanelRegistry.h>

#include <ImGui/imgui_internal.h> // DockBuilder API

#include <Utils/DebugUtils/Debug.h>

#include <algorithm>
#include <cmath>
#include <string>

namespace
{
	// The share of the main dockspace each side column takes. The bottom area has a dockspace of its own (BottomDockArea).
	constexpr float k_leftDockFraction = 0.09f;
	constexpr float k_rightDockFraction = 0.19f;
	// The share of an area its first half (top or left) takes, when panels use both halves
	constexpr float k_firstHalfFraction = 0.5f;

	// The ID ImGui gives a docked window's tab (ImGuiWindow::TabId, "#TAB" in the window's ID scope), so a dock node's
	// shown tab can be picked before the window exists
	ImGuiID GetTabId(const std::string& windowName)
	{
		return ImHashStr("#TAB", 0, ImHashStr(windowName.c_str()));
	}

	// Where the current window's menu bar contents start for them to be centred across it, given how wide they are. Never
	// left of minLeft, where ImGui starts them, so they start there when the bar's too narrow. On a whole pixel, so the
	// text stays crisp.
	float GetCentredMenuBarLeft(float minLeft, float width)
	{
		const ImRect bar = ImGui::GetCurrentWindow()->MenuBarRect();
		return std::max(minLeft, std::floor(bar.GetCenter().x - width * 0.5f));
	}

	// A window's place among the tabs of its dock node, in its settings and, if it exists, the window. ImGui sorts tabs
	// that appear in the same frame by it, and imgui.ini keeps it ("DockId=0x...,<order>").
	void SetDockOrder(ImGuiID windowId, int order)
	{
		if (ImGuiWindowSettings* const settings = ImGui::FindWindowSettingsByID(windowId))
		{
			settings->DockOrder = static_cast<short>(order);
		}
		if (ImGuiWindow* const window = ImGui::FindWindowByID(windowId))
		{
			window->DockOrder = static_cast<short>(order);
		}
	}

	// Whether any window is docked in node or a node under it, counting windows imgui.ini has docked there that haven't
	// been created yet
	bool HasDockedWindows(const ImGuiDockNode& node)
	{
		if (!node.Windows.empty())
		{
			return true;
		}

		ImGuiContext& imgui = *ImGui::GetCurrentContext();
		for (ImGuiWindowSettings* settings = imgui.SettingsWindows.begin(); settings != nullptr; settings = imgui.SettingsWindows.next_chunk(settings))
		{
			for (const ImGuiDockNode* docked = ImGui::DockBuilderGetNode(settings->DockId); docked != nullptr; docked = docked->ParentNode)
			{
				if (docked == &node)
				{
					return true;
				}
			}
		}

		return false;
	}

	// Removes the node if nothing's docked in it, and then each parent left empty. Its sibling is merged into the parent,
	// which ImGui points imgui.ini's windows at too.
	void RemoveEmptyDockNode(ImGuiID nodeId)
	{
		ImGuiDockNode* node = ImGui::DockBuilderGetNode(nodeId);
		while (node != nullptr && node->ParentNode != nullptr && !node->IsCentralNode() && !HasDockedWindows(*node))
		{
			const ImGuiID parentId = node->ParentNode->ID;
			ImGui::DockBuilderRemoveNode(node->ID);
			node = ImGui::DockBuilderGetNode(parentId);
		}
	}

	std::vector<std::string> SplitMenuPath(const std::string& path)
	{
		std::vector<std::string> segments;
		size_t start = 0;
		while (start <= path.size())
		{
			const size_t end = std::min(path.find('/', start), path.size());
			segments.push_back(path.substr(start, end - start));
			start = end + 1;
		}

		return segments;
	}
}

namespace Editor
{
	void PanelManager::CreatePanels(EditorContext& context)
	{
		AssertMsg(m_panels.empty(), "Panels have already been created");

		m_sceneUnloading = context.GetEvents().Subscribe<SceneUnloadingEvent>([this](const SceneUnloadingEvent&)
			{
				for (std::unique_ptr<EditorPanel>& panel : m_panels)
				{
					panel->OnSceneReset();
				}
			});

		for (const PanelRegistration& registration : PanelRegistry::GetAll())
		{
			std::unique_ptr<EditorPanel> panel = registration.Create();
			panel->m_desc = registration.Describe();
			panel->m_isOpen = panel->m_desc.DefaultOpen || !panel->m_desc.Closable;
			m_panels.push_back(std::move(panel));
		}

		// Registration order depends on link order, so sort by name for a stable menu order
		std::sort(m_panels.begin(), m_panels.end(), [](const std::unique_ptr<EditorPanel>& a, const std::unique_ptr<EditorPanel>& b)
			{
				return a->m_desc.Name < b->m_desc.Name;
			});

		for (std::unique_ptr<EditorPanel>& panel : m_panels)
		{
			panel->Initialize(context);
		}
	}

	void PanelManager::UpdateAll(EditorContext& context)
	{
		for (std::unique_ptr<EditorPanel>& panel : m_panels)
		{
			if (panel->m_isOpen)
			{
				panel->Update(context);
			}
		}
	}

	void PanelManager::PreRenderAll(EditorContext& context)
	{
		for (std::unique_ptr<EditorPanel>& panel : m_panels)
		{
			if (panel->m_isOpen)
			{
				panel->PreRender(context);
			}
		}
	}

	void PanelManager::DrawAll(EditorContext& context)
	{
		for (std::unique_ptr<EditorPanel>& panel : m_panels)
		{
			if (!panel->m_isOpen)
			{
				panel->m_isHovered = false;
				panel->m_isFocused = false;
				continue;
			}

			const PanelDesc& desc = panel->m_desc;

			if (desc.OwnsWindow)
			{
				panel->m_isHovered = false;
				panel->m_isFocused = false;
				panel->Draw(context);
				continue;
			}

			PanelWindowStyle windowStyle;
			panel->PushWindowStyle(windowStyle);

			if (desc.DefaultDock == EPanelDock::Floating && desc.DefaultSize.x > 0.0f && desc.DefaultSize.y > 0.0f)
			{
				// Only until imgui.ini has a place and size for it
				ImGui::SetNextWindowSize(desc.DefaultSize, ImGuiCond_FirstUseEver);
				ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
			}
			else
			{
				PlaceUnsavedPanel(*panel);
			}

			if (!desc.AcceptsDocking)
			{
				// Applies to whichever node it's docked in, which then shows no drop targets of its own
				ImGuiWindowClass windowClass;
				windowClass.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoDockingOverMe | ImGuiDockNodeFlags_NoDockingSplit;
				ImGui::SetNextWindowClass(&windowClass);
			}

			// ImGui can only start the menu bar's contents lower, which makes the bar that much taller too. So they're
			// lowered by both paddings, and moved back up by one once the bar begins.
			ImVec2& menuBarOffset = ImGui::GetCurrentContext()->NextWindowData.MenuBarOffsetMinVal;
			menuBarOffset.y = desc.MenuBarPadding * 2.0f;

			bool* const isOpen = desc.Closable ? &panel->m_isOpen : nullptr;
			const bool isVisible = ImGui::Begin(desc.Name.c_str(), isOpen, desc.WindowFlags);
			// ImGui doesn't clear it after use
			menuBarOffset.y = 0.0f;

			panel->m_isHovered = isVisible && ImGui::IsWindowHovered();
			panel->m_isFocused = isVisible && ImGui::IsWindowFocused();

			if (isVisible)
			{
				if ((desc.WindowFlags & ImGuiWindowFlags_MenuBar) != 0 && ImGui::BeginMenuBar())
				{
					DrawPanelMenuBar(*panel, context);
					ImGui::EndMenuBar();
				}

				panel->Draw(context);
			}

			ImGui::End();
			windowStyle.PopAll();
		}
	}

	void PanelManager::DrawPanelMenuBar(EditorPanel& panel, EditorContext& context)
	{
		const PanelDesc& desc = panel.m_desc;
		ImGuiWindow* const window = ImGui::GetCurrentWindow();

		// Where ImGui starts the contents, less the padding it was given below them (see DrawAll)
		const ImVec2 start = ImGui::GetCursorScreenPos();
		// Laid out by last time's width, which is all there is before they're drawn
		const float left = desc.CentreMenuBar ? GetCentredMenuBarLeft(start.x, panel.m_menuBarWidth) : start.x;
		ImGui::SetCursorScreenPos(ImVec2(left, start.y - desc.MenuBarPadding));

		ImDrawList* const drawList = window->DrawList;
		const int firstVertex = drawList->VtxBuffer.Size;
		panel.DrawMenuBar(context);

		if (desc.CentreMenuBar)
		{
			// To the right edge of the last item
			const float width = window->DC.CursorMaxPos.x - left;
			panel.m_menuBarWidth = width;

			// Moved to where this frame's width puts them, so they aren't off-centre for a frame when it changes (and on
			// the first frame). They're clicked where they were laid out until the next frame lays them out there too.
			const float shift = GetCentredMenuBarLeft(start.x, width) - left;
			if (shift != 0.0f)
			{
				for (int i = firstVertex; i < drawList->VtxBuffer.Size; ++i)
				{
					drawList->VtxBuffer[i].pos.x += shift;
				}
			}
		}
	}

	void PanelManager::DrawMenuItems(const char* mainMenu)
	{
		for (std::unique_ptr<EditorPanel>& panel : m_panels)
		{
			const PanelDesc& desc = panel->m_desc;
			if (desc.MenuPath.empty())
			{
				continue;
			}

			// The first segment is the main menu we're already in, the last is the item, and any between are submenus
			const std::vector<std::string> segments = SplitMenuPath(desc.MenuPath);
			if (segments.size() < 2 || segments.front() != mainMenu)
			{
				continue;
			}

			const size_t submenuEnd = segments.size() - 1;

			size_t openedEnd = 1;
			while (openedEnd < submenuEnd && ImGui::BeginMenu(segments[openedEnd].c_str()))
			{
				++openedEnd;
			}

			if (openedEnd == submenuEnd)
			{
				// A panel that can't close stays checked and can't be unchecked
				bool isOpen = panel->m_isOpen;
				if (ImGui::MenuItem(segments.back().c_str(), nullptr, &isOpen, desc.Closable))
				{
					panel->SetOpen(isOpen);
				}
			}

			for (size_t i = 1; i < openedEnd; ++i)
			{
				ImGui::EndMenu();
			}
		}
	}

	void PanelManager::BuildDefaultDockLayout(ImGuiID dockspaceId, const ImVec2& size)
	{
		ImGui::DockBuilderRemoveNode(dockspaceId);
		ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
		ImGui::DockBuilderSetNodeSize(dockspaceId, size);

		// Each column is carved off the remaining central node, which ends up as the Center slot
		ImGuiID slotIds[static_cast<size_t>(EPanelDock::Count)] = {};
		ImGuiID centerId = dockspaceId;
		if (IsSlotUsed(EPanelDock::Left))
		{
			slotIds[static_cast<size_t>(EPanelDock::Left)] = ImGui::DockBuilderSplitNode(centerId, ImGuiDir_Left, k_leftDockFraction, nullptr, &centerId);
		}
		if (IsSlotUsed(EPanelDock::RightTop) || IsSlotUsed(EPanelDock::RightBottom))
		{
			const ImGuiID rightId = ImGui::DockBuilderSplitNode(centerId, ImGuiDir_Right, k_rightDockFraction, nullptr, &centerId);
			SplitArea(rightId, EPanelDock::RightTop, EPanelDock::RightBottom, ImGuiDir_Up,
				slotIds[static_cast<size_t>(EPanelDock::RightTop)], slotIds[static_cast<size_t>(EPanelDock::RightBottom)]);
		}
		slotIds[static_cast<size_t>(EPanelDock::Center)] = centerId;

		for (size_t slot = 0; slot < static_cast<size_t>(EPanelDock::Count); ++slot)
		{
			if (slotIds[slot] != 0)
			{
				DockSlotPanels(static_cast<EPanelDock>(slot), slotIds[slot]);
			}
		}

		ImGui::DockBuilderFinish(dockspaceId);
	}

	void PanelManager::BuildBottomDockLayout(ImGuiID dockspaceId, const ImVec2& size)
	{
		// Where imgui.ini has them, from before the bottom area had a dockspace of its own
		std::vector<ImGuiID> previousNodeIds;
		for (const EPanelDock slot : { EPanelDock::BottomLeft, EPanelDock::BottomRight })
		{
			for (const EditorPanel* const panel : GetPanelsInSlot(slot))
			{
				if (const ImGuiWindowSettings* const settings = ImGui::FindWindowSettingsByID(ImHashStr(panel->m_desc.Name.c_str())))
				{
					previousNodeIds.push_back(settings->DockId);
				}
			}
		}

		ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
		ImGui::DockBuilderSetNodeSize(dockspaceId, size);

		ImGuiID leftId = 0;
		ImGuiID rightId = 0;
		SplitArea(dockspaceId, EPanelDock::BottomLeft, EPanelDock::BottomRight, ImGuiDir_Left, leftId, rightId);
		if (leftId != 0)
		{
			DockSlotPanels(EPanelDock::BottomLeft, leftId);
		}
		if (rightId != 0)
		{
			DockSlotPanels(EPanelDock::BottomRight, rightId);
		}

		// So the main dockspace doesn't keep an empty space where they were
		for (const ImGuiID nodeId : previousNodeIds)
		{
			RemoveEmptyDockNode(nodeId);
		}

		ImGui::DockBuilderFinish(dockspaceId);
	}

	void PanelManager::SplitArea(ImGuiID areaId, EPanelDock first, EPanelDock second, ImGuiDir firstDir, ImGuiID& firstId, ImGuiID& secondId) const
	{
		const bool isFirstUsed = IsSlotUsed(first);
		const bool isSecondUsed = IsSlotUsed(second);
		if (isFirstUsed && isSecondUsed)
		{
			firstId = ImGui::DockBuilderSplitNode(areaId, firstDir, k_firstHalfFraction, nullptr, &secondId);
		}
		else if (isFirstUsed)
		{
			firstId = areaId;
		}
		else if (isSecondUsed)
		{
			secondId = areaId;
		}
	}

	bool PanelManager::IsSlotUsed(EPanelDock slot) const
	{
		return std::any_of(m_panels.begin(), m_panels.end(), [slot](const std::unique_ptr<EditorPanel>& panel)
			{
				return panel->m_desc.DefaultDock == slot;
			});
	}

	void PanelManager::DockSlotPanels(EPanelDock slot, ImGuiID nodeId) const
	{
		const std::vector<const EditorPanel*> panels = GetPanelsInSlot(slot);
		if (panels.empty())
		{
			return;
		}

		for (size_t order = 0; order < panels.size(); ++order)
		{
			const std::string& name = panels[order]->m_desc.Name;
			ImGui::DockBuilderDockWindow(name.c_str(), nodeId);
			SetDockOrder(ImHashStr(name.c_str()), static_cast<int>(order));
		}
		ImGui::DockBuilderGetNode(nodeId)->SelectedTabId = GetTabId(panels.front()->m_desc.Name);
	}

	std::vector<const EditorPanel*> PanelManager::GetPanelsInSlot(EPanelDock slot) const
	{
		std::vector<const EditorPanel*> panels;
		if (slot == EPanelDock::Floating)
		{
			return panels;
		}

		// m_panels is sorted by name; panels that start open come first, so they take the leading tabs
		for (const bool isDefaultOpen : { true, false })
		{
			for (const std::unique_ptr<EditorPanel>& panel : m_panels)
			{
				const PanelDesc& desc = panel->m_desc;
				if (desc.DefaultDock == slot && !desc.OwnsWindow && desc.DefaultOpen == isDefaultOpen)
				{
					panels.push_back(panel.get());
				}
			}
		}

		return panels;
	}

	void PanelManager::PlaceUnsavedPanel(const EditorPanel& panel)
	{
		const PanelDesc& desc = panel.m_desc;
		const ImGuiID windowId = ImHashStr(desc.Name.c_str());
		// Once its window exists, or once imgui.ini has it (BuildDefaultDockLayout and BuildBottomDockLayout place every
		// docked panel), ImGui
		// already knows where it goes
		if (desc.DefaultDock == EPanelDock::Floating || desc.OwnsWindow || ImGui::FindWindowByID(windowId) != nullptr
			|| ImGui::FindWindowSettingsByID(windowId) != nullptr)
		{
			return;
		}

		// The node of the first saved panel from its slot, and the first saved panel in that node that comes after it
		const std::vector<const EditorPanel*> slotPanels = GetPanelsInSlot(desc.DefaultDock);
		const auto self = std::find(slotPanels.begin(), slotPanels.end(), &panel);
		ImGuiID dockId = 0;
		const ImGuiWindowSettings* next = nullptr;
		for (auto it = slotPanels.begin(); it != slotPanels.end(); ++it)
		{
			const ImGuiWindowSettings* const settings = ImGui::FindWindowSettingsByID(ImHashStr((*it)->m_desc.Name.c_str()));
			if (settings == nullptr || settings->DockId == 0 || (dockId != 0 && settings->DockId != dockId))
			{
				continue;
			}

			dockId = settings->DockId;
			if (it > self && settings->DockOrder >= 0)
			{
				next = settings;
				break;
			}
		}
		if (dockId == 0)
		{
			return;
		}

		// Takes the place of the panel after it, moving that and every later tab in the node along one, or goes last.
		// Tabs the user has moved stay in their order.
		ImGuiContext& imgui = *ImGui::GetCurrentContext();
		int order = 0;
		if (next != nullptr)
		{
			order = next->DockOrder;
			for (ImGuiWindowSettings* settings = imgui.SettingsWindows.begin(); settings != nullptr; settings = imgui.SettingsWindows.next_chunk(settings))
			{
				if (settings->DockId == dockId && settings->DockOrder >= order)
				{
					SetDockOrder(settings->ID, settings->DockOrder + 1);
				}
			}
		}
		else
		{
			for (ImGuiWindowSettings* settings = imgui.SettingsWindows.begin(); settings != nullptr; settings = imgui.SettingsWindows.next_chunk(settings))
			{
				if (settings->DockId == dockId)
				{
					order = std::max(order, settings->DockOrder + 1);
				}
			}
		}

		ImGuiWindowSettings* const settings = ImGui::CreateNewWindowSettings(desc.Name.c_str());
		settings->DockId = dockId;
		settings->DockOrder = static_cast<short>(order);

		// Shown, so a panel that's just been added is seen
		if (ImGuiDockNode* const node = ImGui::DockBuilderGetNode(dockId))
		{
			node->SelectedTabId = GetTabId(desc.Name);
		}
	}
}
