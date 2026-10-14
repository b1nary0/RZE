#pragma once

#include <UI/Framework/EditorPanel.h>
#include <UI/Settings/SettingsPage.h>

#include <memory>
#include <vector>

namespace Editor
{
	// The Settings window (Edit > Settings): the registered SettingsPages listed down the left, and the chosen one
	// on the right
	class SettingsPanel : public EditorPanel
	{
	public:
		static PanelDesc Describe();

	private:
		void Initialize(EditorContext& context) override;
		void Draw(EditorContext& context) override;

		void DrawPageList();
		void DrawPage(EditorContext& context);

	private:
		std::vector<std::unique_ptr<SettingsPage>> m_pages;
		size_t m_selectedPage = 0;
	};
}
