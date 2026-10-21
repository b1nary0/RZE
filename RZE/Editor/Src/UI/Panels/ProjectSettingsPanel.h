#pragma once

#include <UI/Framework/EditorPanel.h>

#include <EngineCore/Config/GameConfig.h>
#include <Game/World/GameObject/Properties/PropertyDescriptor.h>

#include <array>
#include <string>

namespace Editor
{
	// The Project Settings window (Edit > Project Settings): what the game itself is, saved to Config/Game.ini as it's
	// changed. Unlike Settings, which are the editor's own and last until it closes.
	class ProjectSettingsPanel : public EditorPanel
	{
	public:
		static PanelDesc Describe();

	private:
		void Initialize(EditorContext& context) override;
		void Draw(EditorContext& context) override;

		void DrawIcon(EditorContext& context);
		// Sets and saves the game's icon, and makes its previews
		void SetIcon(const Filepath& icon);
		// The icon as the game's window will show it, at each preview size
		void MakePreviews();

	private:
		GameConfig m_gameConfig;
		// The icon row, drawn by the Object Inspector's asset path drawer
		PropertyDescriptor m_iconProperty;

		// The icon made square and scaled as the game does it, at k_previewSizes; empty textures without one
		std::array<PropertyTexture, 3> m_previews;
		// At a large size, for the row's thumbnail and its hover preview
		PropertyTexture m_thumbnail;
		// The chosen image's own size, for the notes under the previews; 0 x 0 if it couldn't be read
		int m_iconWidth = 0;
		int m_iconHeight = 0;

		// Why the last save failed; empty once one works
		std::string m_saveError;
	};
}
