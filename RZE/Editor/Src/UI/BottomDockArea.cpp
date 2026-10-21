#include <UI/BottomDockArea.h>

#include <UI/Framework/EditorContext.h>
#include <UI/Framework/EditorUI.h>

#include <ImGui/imgui_internal.h> // BeginViewportSideBar, ImGuiViewportP, SplitterBehavior, DockBuilder API, settings handlers

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace
{
	constexpr char k_windowName[] = "##BottomDockArea";
	constexpr char k_stripName[] = "##BottomDockStrip";
	constexpr char k_dockspaceName[] = "BottomDockspace";

	// imgui.ini's [BottomDockArea][Area]
	constexpr char k_settingsTypeName[] = "BottomDockArea";
	constexpr char k_settingsEntryName[] = "Area";
	constexpr char k_pinnedKey[] = "Pinned=";
	constexpr char k_heightKey[] = "Height=";

	// The share of the work area it starts with
	constexpr float k_defaultHeightFraction = 0.27f;
	// The splitter never makes it, or what's left above it, shorter than these. Pixels.
	constexpr float k_minHeight = 60.0f;
	constexpr float k_minHeightAbove = 120.0f;
	// How far past its edges the splitter answers the mouse, and how long it's hovered before the cursor changes, as for
	// ImGui's own splitters between dock nodes
	constexpr float k_splitterHoverExtend = 4.0f;
	constexpr float k_splitterHoverDelay = 0.04f;

	// Around the strip's tabs. Pixels.
	constexpr float k_stripEdgePadding = 4.0f;
	constexpr float k_stripVerticalPadding = 2.0f;

	// The dockspace's leaf nodes, left to right and top to bottom
	void CollectLeafNodes(ImGuiDockNode* node, std::vector<ImGuiDockNode*>& leaves)
	{
		if (node == nullptr)
		{
			return;
		}
		if (node->IsLeafNode())
		{
			leaves.push_back(node);
			return;
		}
		CollectLeafNodes(node->ChildNodes[0], leaves);
		CollectLeafNodes(node->ChildNodes[1], leaves);
	}

	// A leaf node's windows in their tab order
	std::vector<ImGuiWindow*> GetNodeWindows(const ImGuiDockNode& node)
	{
		std::vector<ImGuiWindow*> windows;
		if (node.TabBar != nullptr)
		{
			for (const ImGuiTabItem& tab : node.TabBar->Tabs)
			{
				// Buttons added to the tab bar, like the pin, have no window
				if (tab.Window != nullptr)
				{
					windows.push_back(tab.Window);
				}
			}
		}
		else
		{
			windows.assign(node.Windows.begin(), node.Windows.end());
		}

		return windows;
	}

	// Along the top of tab, as ImGui draws one on the selected tab of a focused dock node's tab bar
	void DrawTabOverline(const ImRect& tab)
	{
		const ImGuiStyle& style = ImGui::GetStyle();
		if (style.TabBarOverlineSize <= 0.0f)
		{
			return;
		}

		ImDrawList* const drawList = ImGui::GetWindowDrawList();
		const ImU32 colour = ImGui::GetColorU32(ImGuiCol_TabSelectedOverline);
		const float top = tab.Min.y + ImGui::GetCurrentContext()->CurrentDpiScale;
		if (style.TabRounding > 0.0f)
		{
			const float rounding = style.TabRounding;
			drawList->PathArcToFast(ImVec2(tab.Min.x + rounding, top + rounding), rounding, 7, 9);
			drawList->PathArcToFast(ImVec2(tab.Max.x - rounding, top + rounding), rounding, 9, 11);
			drawList->PathStroke(colour, ImDrawFlags_None, style.TabBarOverlineSize);
		}
		else
		{
			drawList->AddLine(ImVec2(tab.Min.x - 0.5f, top - 0.5f), ImVec2(tab.Max.x - 0.5f, top - 0.5f), colour, style.TabBarOverlineSize);
		}
	}

	// Makes window its node's shown tab. ImGui doesn't when it focuses a docked window.
	void SelectTab(ImGuiWindow& window)
	{
		ImGuiDockNode* const node = window.DockNode;
		if (node == nullptr)
		{
			return;
		}

		node->SelectedTabId = window.TabId;
		if (node->TabBar != nullptr)
		{
			node->TabBar->NextSelectedTabId = window.TabId;
		}
	}
}

namespace Editor
{
	void BottomDockArea::Initialize()
	{
		ImGuiSettingsHandler handler;
		handler.TypeName = k_settingsTypeName;
		handler.TypeHash = ImHashStr(k_settingsTypeName);
		handler.ReadOpenFn = &BottomDockArea::ReadSettingsOpen;
		handler.ReadLineFn = &BottomDockArea::ReadSettingsLine;
		handler.WriteAllFn = &BottomDockArea::WriteSettings;
		handler.UserData = this;
		ImGui::AddSettingsHandler(&handler);
	}

	void BottomDockArea::Draw(EditorContext& context)
	{
		ImGuiViewportP* const viewport = static_cast<ImGuiViewportP*>(ImGui::GetMainViewport());
		const ImGuiID dockspaceId = ImHashStr(k_dockspaceName);
		const float splitterHeight = ImGui::GetStyle().DockingSeparatorSize;

		if (m_height <= 0.0f)
		{
			// What the menu bar and status bar have left this frame
			m_height = std::round(viewport->GetBuildWorkRect().GetHeight() * k_defaultHeightFraction);
		}

		if (ImGui::DockBuilderGetNode(dockspaceId) == nullptr)
		{
			context.GetPanels().BuildBottomDockLayout(dockspaceId, ImVec2(viewport->WorkSize.x, m_height));
		}

		const ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoSavedSettings
			| ImGuiWindowFlags_NoNavFocus;

		if (m_isPinned)
		{
			const float workHeight = viewport->GetBuildWorkRect().GetHeight();
			const float height = std::clamp(m_height, k_minHeight, std::max(k_minHeight, workHeight - splitterHeight - k_minHeightAbove));

			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
			const bool isVisible = ImGui::BeginViewportSideBar(k_windowName, viewport, ImGuiDir_Down, splitterHeight + height,
				flags | ImGuiWindowFlags_NoBringToFrontOnFocus);
			ImGui::PopStyleVar(2);

			if (isVisible)
			{
				// The side bar has taken its height out of the work area
				DrawArea(dockspaceId, height, viewport->GetBuildWorkRect().GetHeight());
			}
			// Like Begin, whether or not it's visible
			ImGui::End();
			return;
		}

		// Clicking the strip takes focus from the area, so while it has focus the area stays as it was; DrawStrip decides
		// what a click on one of its tabs does. A drag that started in the area while it was open, such as an asset on
		// its way to the scene view, keeps it open until it's dropped.
		const bool wasOpen = m_isOpen;
		const ImGuiWindow* const navWindow = ImGui::GetCurrentContext()->NavWindow;
		const bool isStripFocused = navWindow != nullptr && navWindow == ImGui::FindWindowByName(k_stripName);
		m_isOpen = IsFocusInArea(dockspaceId) || (wasOpen && (isStripFocused || ImGui::GetDragDropPayload() != nullptr));

		// Until its dock nodes have been drawn once, ImGui doesn't treat its panels as docked, but draws them as floating
		// windows, so on its first frames closed (e.g. starting in auto-hide) it's drawn out of sight below the viewport
		const bool isDrawnOutOfSight = !m_isOpen && !IsHosted(dockspaceId);

		DrawStrip(dockspaceId, isDrawnOutOfSight);
		if (!m_isOpen && !isDrawnOutOfSight)
		{
			return;
		}

		// Over the bottom of the main dockspace, just above the strip
		const ImRect work = viewport->GetBuildWorkRect();
		const float height = std::clamp(m_height, k_minHeight, std::max(k_minHeight, work.GetHeight() - splitterHeight - k_minHeightAbove));
		const float top = isDrawnOutOfSight ? viewport->Pos.y + viewport->Size.y : work.Max.y - splitterHeight - height;
		ImGui::SetNextWindowPos(ImVec2(work.Min.x, top));
		ImGui::SetNextWindowSize(ImVec2(work.GetWidth(), splitterHeight + height));
		ImGui::SetNextWindowViewport(viewport->ID);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(0.0f, 0.0f));
		// It appears each time it opens. Not taking focus as it does leaves it with the panel it was opened on.
		const bool isVisible = ImGui::Begin(k_windowName, nullptr,
			flags | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking
				| ImGuiWindowFlags_NoFocusOnAppearing);
		ImGui::PopStyleVar(4);

		// In front of the main dockspace's panels. Only as it opens: popups opened from its panels go in front of it.
		if (m_isOpen && !wasOpen)
		{
			ImGui::BringWindowToDisplayFront(ImGui::GetCurrentWindow());
		}

		if (isVisible)
		{
			DrawArea(dockspaceId, height, work.GetHeight() - splitterHeight - height);
		}
		ImGui::End();
	}

	void BottomDockArea::DrawArea(ImGuiID dockspaceId, float height, float heightAbove)
	{
		const float splitterHeight = ImGui::GetStyle().DockingSeparatorSize;
		const ImVec2 position = ImGui::GetWindowPos();
		const float width = ImGui::GetWindowWidth();

		ImGui::SetCursorScreenPos(ImVec2(position.x, position.y + splitterHeight));
		ImGui::DockSpace(dockspaceId, ImVec2(width, height));

		// At the end of the top right node's tab bar, should the area be split
		ImGuiDockNode* node = ImGui::DockBuilderGetNode(dockspaceId);
		while (node != nullptr && node->IsSplitNode())
		{
			node = node->ChildNodes[node->SplitAxis == ImGuiAxis_X ? 1 : 0];
		}
		if (node != nullptr && ImGui::DockNodeBeginAmendTabBar(node))
		{
			DrawPinButton(node->TabBar->BarRect);
			ImGui::DockNodeEndAmendTabBar();
		}

		// After the dockspace, so it wins the mouse over the top of the tab bar it reaches into
		const ImRect splitter(position, ImVec2(position.x + width, position.y + splitterHeight));
		if (ImGui::SplitterBehavior(splitter, ImGui::GetID("##Splitter"), ImGuiAxis_Y, &heightAbove, &height,
			k_minHeightAbove, k_minHeight, k_splitterHoverExtend, k_splitterHoverDelay))
		{
			m_height = std::round(height);
			ImGui::MarkIniSettingsDirty();
		}
	}

	void BottomDockArea::DrawStrip(ImGuiID dockspaceId, bool isAreaDrawn)
	{
		const float height = ImGui::GetFrameHeight() + k_stripVerticalPadding * 2.0f;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(k_stripEdgePadding, k_stripVerticalPadding));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		const bool isVisible = ImGui::BeginViewportSideBar(k_stripName, ImGui::GetMainViewport(), ImGuiDir_Down, height,
			ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus
			| ImGuiWindowFlags_NoNavFocus);
		ImGui::PopStyleVar(2);

		if (isVisible && ImGui::BeginTabBar("##Tabs", ImGuiTabBarFlags_NoTooltip | ImGuiTabBarFlags_FittingPolicyScroll))
		{
			// The panel focus is in, or that a focused popup was opened from
			const ImGuiWindow* const navWindow = ImGui::GetCurrentContext()->NavWindow;
			const ImGuiWindow* const focusedPanel = navWindow != nullptr ? navWindow->RootWindowPopupTree->RootWindow : nullptr;

			std::vector<ImGuiDockNode*> leaves;
			CollectLeafNodes(ImGui::DockBuilderGetNode(dockspaceId), leaves);
			for (const ImGuiDockNode* const node : leaves)
			{
				for (ImGuiWindow* const window : GetNodeWindows(*node))
				{
					// While it's open, the tabs it's showing look as their tabs do in the area: selected, in a focused tab
					// bar for the focused one, with its overline
					const bool isShown = m_isOpen && node->SelectedTabId == window->TabId;
					const bool isFocused = isShown && window == focusedPanel;
					if (isShown)
					{
						ImGui::PushStyleColor(ImGuiCol_Tab, ImGui::GetStyleColorVec4(isFocused ? ImGuiCol_TabSelected : ImGuiCol_TabDimmedSelected));
					}
					const bool isClicked = ImGui::TabItemButton(window->Name);
					if (isShown)
					{
						ImGui::PopStyleColor();
					}
					if (isFocused)
					{
						DrawTabOverline(ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax()));
					}

					if (isClicked)
					{
						// The one it's showing closes it; any other opens it on that panel, or switches to it
						m_isOpen = !isShown;
						if (m_isOpen)
						{
							SelectTab(*window);
							ImGui::FocusWindow(window);
						}
					}
				}
			}

			ImGui::EndTabBar();
		}
		// While it's open, the pin's at the end of the area's own tab bar instead
		if (isVisible && !m_isOpen)
		{
			DrawPinButton(ImGui::GetCurrentWindow()->WorkRect);
		}

		// Its panels stay docked while it's closed, though nothing draws them
		if (!m_isOpen && !isAreaDrawn)
		{
			ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_KeepAliveOnly);
		}
		ImGui::End();
	}

	void BottomDockArea::DrawPinButton(const ImRect& bar)
	{
		const float size = EditorUI::GetIconButtonWidth();
		ImGui::SetCursorScreenPos(ImVec2(bar.Max.x - size, bar.Min.y + (bar.GetHeight() - size) * 0.5f));
		if (EditorUI::IconButton("##Pin", m_isPinned ? EditorUI::EIcon::Pin : EditorUI::EIcon::Unpin,
			m_isPinned ? "Auto-hide: shrink to tabs when not in use" : "Pin: keep open"))
		{
			SetPinned(!m_isPinned);
		}
	}

	void BottomDockArea::SetPinned(bool isPinned)
	{
		if (isPinned == m_isPinned)
		{
			return;
		}

		m_isPinned = isPinned;
		ImGui::MarkIniSettingsDirty();

		if (m_isPinned)
		{
			// Back behind the floating windows, as it opened in front of everything
			if (ImGuiWindow* const window = FindWindow())
			{
				ImGui::BringWindowToDisplayBack(window);
			}
		}
		else
		{
			// Straight down to the strip, rather than staying open until focus goes elsewhere
			m_isOpen = false;
			if (IsFocusInArea(ImHashStr(k_dockspaceName)))
			{
				ImGui::FocusWindow(nullptr);
			}
		}
	}

	bool BottomDockArea::IsFocusInArea(ImGuiID dockspaceId) const
	{
		const ImGuiWindow* const navWindow = ImGui::GetCurrentContext()->NavWindow;
		if (navWindow == nullptr)
		{
			return false;
		}

		// Its own window or one inside it (the dockspace's host, which clicking the tab bar's empty space or the pin
		// focuses; a panel docked in it, or a child of one), or a popup opened from any of them. A panel hidden while
		// the area's closed isn't inside its window, but is still in its dock nodes.
		ImGuiWindow* const root = navWindow->RootWindowPopupTree;
		if (root->RootWindowDockTree == FindWindow())
		{
			return true;
		}
		return root->DockNode != nullptr && ImGui::DockNodeGetRootNode(root->DockNode)->ID == dockspaceId;
	}

	bool BottomDockArea::IsHosted(ImGuiID dockspaceId) const
	{
		std::vector<ImGuiDockNode*> leaves;
		CollectLeafNodes(ImGui::DockBuilderGetNode(dockspaceId), leaves);
		return std::all_of(leaves.begin(), leaves.end(), [](const ImGuiDockNode* node) { return node->HostWindow != nullptr; });
	}

	ImGuiWindow* BottomDockArea::FindWindow() const
	{
		return ImGui::FindWindowByName(k_windowName);
	}

	void* BottomDockArea::ReadSettingsOpen(ImGuiContext*, ImGuiSettingsHandler* handler, const char* name)
	{
		return std::strcmp(name, k_settingsEntryName) == 0 ? handler->UserData : nullptr;
	}

	void BottomDockArea::ReadSettingsLine(ImGuiContext*, ImGuiSettingsHandler*, void* entry, const char* line)
	{
		BottomDockArea& area = *static_cast<BottomDockArea*>(entry);
		if (std::strncmp(line, k_pinnedKey, std::strlen(k_pinnedKey)) == 0)
		{
			area.m_isPinned = std::strtol(line + std::strlen(k_pinnedKey), nullptr, 10) != 0;
		}
		else if (std::strncmp(line, k_heightKey, std::strlen(k_heightKey)) == 0)
		{
			area.m_height = std::max(0.0f, std::strtof(line + std::strlen(k_heightKey), nullptr));
		}
	}

	void BottomDockArea::WriteSettings(ImGuiContext*, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buffer)
	{
		const BottomDockArea& area = *static_cast<const BottomDockArea*>(handler->UserData);
		buffer->appendf("[%s][%s]\n", k_settingsTypeName, k_settingsEntryName);
		buffer->appendf("%s%d\n", k_pinnedKey, area.m_isPinned ? 1 : 0);
		if (area.m_height > 0.0f)
		{
			buffer->appendf("%s%d\n", k_heightKey, static_cast<int>(area.m_height));
		}
		buffer->append("\n");
	}
}
