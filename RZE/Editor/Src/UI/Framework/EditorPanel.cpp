#include <UI/Framework/EditorPanel.h>

namespace Editor
{
	void PanelWindowStyle::Push(ImGuiStyleVar var, float value)
	{
		ImGui::PushStyleVar(var, value);
		++m_varCount;
	}

	void PanelWindowStyle::Push(ImGuiStyleVar var, const ImVec2& value)
	{
		ImGui::PushStyleVar(var, value);
		++m_varCount;
	}

	void PanelWindowStyle::Push(ImGuiCol colour, const ImVec4& value)
	{
		ImGui::PushStyleColor(colour, value);
		++m_colourCount;
	}

	void PanelWindowStyle::PopAll()
	{
		if (m_varCount > 0)
		{
			ImGui::PopStyleVar(m_varCount);
			m_varCount = 0;
		}

		if (m_colourCount > 0)
		{
			ImGui::PopStyleColor(m_colourCount);
			m_colourCount = 0;
		}
	}
}
