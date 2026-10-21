#include <UI/BottomDockArea.h>

#include <UI/Framework/EditorContext.h>

#include <ImGui/imgui_internal.h> // BeginViewportSideBar, ImGuiViewportP, SplitterBehavior, DockBuilder API

#include <algorithm>
#include <cmath>

namespace
{
	constexpr char k_windowName[] = "##BottomDockArea";
	constexpr char k_dockspaceName[] = "BottomDockspace";

	// The share of the work area it starts with
	constexpr float k_defaultHeightFraction = 0.22f;
	// The splitter never makes it, or what's left above it, shorter than these. Pixels.
	constexpr float k_minHeight = 60.0f;
	constexpr float k_minHeightAbove = 120.0f;
	// How far past its edges the splitter answers the mouse, and how long it's hovered before the cursor changes, as for
	// ImGui's own splitters between dock nodes
	constexpr float k_splitterHoverExtend = 4.0f;
	constexpr float k_splitterHoverDelay = 0.04f;
}

namespace Editor
{
	void BottomDockArea::Draw(EditorContext& context)
	{
		ImGuiViewportP* const viewport = static_cast<ImGuiViewportP*>(ImGui::GetMainViewport());
		// What the menu bar and status bar have left this frame
		const float workHeight = viewport->GetBuildWorkRect().GetHeight();
		const float splitterHeight = ImGui::GetStyle().DockingSeparatorSize;

		if (m_height <= 0.0f)
		{
			m_height = std::round(workHeight * k_defaultHeightFraction);
		}
		const float maxHeight = std::max(k_minHeight, workHeight - splitterHeight - k_minHeightAbove);
		float height = std::clamp(m_height, k_minHeight, maxHeight);

		const ImGuiID dockspaceId = ImHashStr(k_dockspaceName);
		if (ImGui::DockBuilderGetNode(dockspaceId) == nullptr)
		{
			context.GetPanels().BuildBottomDockLayout(dockspaceId, ImVec2(viewport->WorkSize.x, height));
		}

		const ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoSavedSettings
			| ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		const bool isOpen = ImGui::BeginViewportSideBar(k_windowName, viewport, ImGuiDir_Down, splitterHeight + height, flags);
		ImGui::PopStyleVar(2);

		if (isOpen)
		{
			const ImVec2 position = ImGui::GetWindowPos();
			const float width = ImGui::GetWindowWidth();

			ImGui::SetCursorScreenPos(ImVec2(position.x, position.y + splitterHeight));
			ImGui::DockSpace(dockspaceId, ImVec2(width, height));

			// After the dockspace, so it wins the mouse over the top of the tab bar it reaches into
			const ImRect splitter(position, ImVec2(position.x + width, position.y + splitterHeight));
			float heightAbove = viewport->GetBuildWorkRect().GetHeight();
			if (ImGui::SplitterBehavior(splitter, ImGui::GetID("##Splitter"), ImGuiAxis_Y, &heightAbove, &height,
				k_minHeightAbove, k_minHeight, k_splitterHoverExtend, k_splitterHoverDelay))
			{
				m_height = std::round(height);
			}
		}
		// Like Begin, whether or not it's open
		ImGui::End();
	}
}
