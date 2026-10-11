#include <UI/Framework/EditorPanel.h>

namespace Editor
{
	void PanelWindowStyle::Push(ImGuiStyleVar var, float value)
	{
		ImGui::PushStyleVar(var, value);
		++m_pushCount;
	}

	void PanelWindowStyle::Push(ImGuiStyleVar var, const ImVec2& value)
	{
		ImGui::PushStyleVar(var, value);
		++m_pushCount;
	}

	void PanelWindowStyle::PopAll()
	{
		if (m_pushCount > 0)
		{
			ImGui::PopStyleVar(m_pushCount);
			m_pushCount = 0;
		}
	}
}
