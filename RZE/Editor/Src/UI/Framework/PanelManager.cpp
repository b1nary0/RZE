#include <UI/Framework/PanelManager.h>

#include <UI/Framework/PanelRegistry.h>

#include <ImGui/imgui_internal.h> // DockBuilder API

#include <Utils/DebugUtils/Debug.h>

#include <algorithm>
#include <string>

namespace
{
	// The share of the dockspace each side slot takes. Bottom is split off first, so it spans the full width.
	constexpr float k_bottomDockFraction = 0.22f;
	constexpr float k_leftDockFraction = 0.09f;
	constexpr float k_rightDockFraction = 0.12f;

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

			bool* const isOpen = desc.Closable ? &panel->m_isOpen : nullptr;
			const bool isVisible = ImGui::Begin(desc.Name.c_str(), isOpen, desc.WindowFlags);

			panel->m_isHovered = isVisible && ImGui::IsWindowHovered();
			panel->m_isFocused = isVisible && ImGui::IsWindowFocused();

			if (isVisible)
			{
				if ((desc.WindowFlags & ImGuiWindowFlags_MenuBar) != 0 && ImGui::BeginMenuBar())
				{
					panel->DrawMenuBar(context);
					ImGui::EndMenuBar();
				}

				panel->Draw(context);
			}

			ImGui::End();
			windowStyle.PopAll();
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
		auto isSlotUsed = [this](EPanelDock slot)
			{
				return std::any_of(m_panels.begin(), m_panels.end(), [slot](const std::unique_ptr<EditorPanel>& panel)
					{
						return panel->m_desc.DefaultDock == slot;
					});
			};

		ImGui::DockBuilderRemoveNode(dockspaceId);
		ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
		ImGui::DockBuilderSetNodeSize(dockspaceId, size);

		// Each split carves a slot off the remaining central node, which ends up as the Center slot
		ImGuiID slotIds[static_cast<size_t>(EPanelDock::Count)] = {};
		ImGuiID centerId = dockspaceId;
		if (isSlotUsed(EPanelDock::Bottom))
		{
			slotIds[static_cast<size_t>(EPanelDock::Bottom)] = ImGui::DockBuilderSplitNode(centerId, ImGuiDir_Down, k_bottomDockFraction, nullptr, &centerId);
		}
		if (isSlotUsed(EPanelDock::Left))
		{
			slotIds[static_cast<size_t>(EPanelDock::Left)] = ImGui::DockBuilderSplitNode(centerId, ImGuiDir_Left, k_leftDockFraction, nullptr, &centerId);
		}
		if (isSlotUsed(EPanelDock::Right))
		{
			slotIds[static_cast<size_t>(EPanelDock::Right)] = ImGui::DockBuilderSplitNode(centerId, ImGuiDir_Right, k_rightDockFraction, nullptr, &centerId);
		}
		slotIds[static_cast<size_t>(EPanelDock::Center)] = centerId;

		// Panels that start open first, so they take the leading tabs in a shared slot
		for (const bool isDefaultOpen : { true, false })
		{
			for (const std::unique_ptr<EditorPanel>& panel : m_panels)
			{
				const PanelDesc& desc = panel->m_desc;
				if (desc.DefaultDock != EPanelDock::Floating && !desc.OwnsWindow && desc.DefaultOpen == isDefaultOpen)
				{
					ImGui::DockBuilderDockWindow(desc.Name.c_str(), slotIds[static_cast<size_t>(desc.DefaultDock)]);
				}
			}
		}

		ImGui::DockBuilderFinish(dockspaceId);
	}

	void PanelManager::ResetSceneState()
	{
		for (std::unique_ptr<EditorPanel>& panel : m_panels)
		{
			panel->OnSceneReset();
		}
	}
}
