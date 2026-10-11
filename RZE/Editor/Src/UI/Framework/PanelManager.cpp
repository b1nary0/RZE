#include <UI/Framework/PanelManager.h>

#include <UI/Framework/PanelRegistry.h>

#include <Utils/DebugUtils/Debug.h>

#include <algorithm>
#include <string>

namespace
{
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

			PanelWindowStyle windowStyle;
			panel->PushWindowStyle(windowStyle);

			bool* const isOpen = desc.Closable ? &panel->m_isOpen : nullptr;
			const bool isVisible = ImGui::Begin(desc.Name.c_str(), isOpen, desc.WindowFlags);

			panel->m_isHovered = isVisible && ImGui::IsWindowHovered();
			panel->m_isFocused = isVisible && ImGui::IsWindowFocused();

			if (isVisible)
			{
				panel->Draw(context);
			}

			ImGui::End();
			windowStyle.PopAll();
		}
	}

	void PanelManager::DrawMenuItems()
	{
		for (std::unique_ptr<EditorPanel>& panel : m_panels)
		{
			const PanelDesc& desc = panel->m_desc;
			if (desc.MenuPath.empty())
			{
				continue;
			}

			// Every segment but the last is a submenu; the last is the item
			const std::vector<std::string> segments = SplitMenuPath(desc.MenuPath);
			const size_t submenuCount = segments.size() - 1;

			size_t openedCount = 0;
			while (openedCount < submenuCount && ImGui::BeginMenu(segments[openedCount].c_str()))
			{
				++openedCount;
			}

			if (openedCount == submenuCount)
			{
				// A panel that can't close stays checked and can't be unchecked
				bool isOpen = panel->m_isOpen;
				if (ImGui::MenuItem(segments.back().c_str(), nullptr, &isOpen, desc.Closable))
				{
					panel->SetOpen(isOpen);
				}
			}

			for (size_t i = 0; i < openedCount; ++i)
			{
				ImGui::EndMenu();
			}
		}
	}

	void PanelManager::ResetSceneState()
	{
		for (std::unique_ptr<EditorPanel>& panel : m_panels)
		{
			panel->OnSceneReset();
		}
	}
}
