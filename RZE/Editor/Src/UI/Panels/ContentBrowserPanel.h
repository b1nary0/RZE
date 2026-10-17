#pragma once

#include <UI/Framework/EditorEvents.h>
#include <UI/Framework/EditorPanel.h>
#include <UI/Widgets/FileTileGrid.h>

#include <Services/FolderListing.h>

#include <Utils/Platform/Filepath.h>

#include <chrono>
#include <string>
#include <unordered_map>
#include <vector>

namespace Editor
{
	// Browses the project's Assets and ProjectData folders: a folder tree beside a grid of tiles like the file
	// browser's, with back, forward and up (also the mouse's side buttons and Backspace), search, and right-click
	// actions on files. Double-clicking a folder opens it, and a scene loads it.
	class ContentBrowserPanel : public EditorPanel
	{
	public:
		static PanelDesc Describe();

	private:
		void Initialize(EditorContext& context) override;
		void Draw(EditorContext& context) override;

		void DrawToolbar();
		void DrawFolderTree();
		void DrawFolderNode(const Filepath& folder);
		void DrawTiles(EditorContext& context);
		void HandleNavigationInput(EditorContext& context);

		// Goes to folder, adding where it was to the back history
		void Navigate(const Filepath& folder);
		void GoBack();
		void GoForward();
		void GoUp();
		// Shows folder without touching the history
		void ShowFolder(const Filepath& folder);
		// Reads the current folder and the tree's folders again
		void Refresh();

		// Opens a folder, or loads a scene; anything else stays selected
		void Open(EditorContext& context, const Filepath& file);
		// Asks about unsaved changes first, like File > Load Scene
		void LoadScene(EditorContext& context, const Filepath& scene);

		// The root the current folder is in
		const Filepath& GetRoot() const;
		bool IsRoot(const Filepath& folder) const;
		// Read the first time a tree node needs them, then kept until Refresh
		const std::vector<Filepath>& GetSubfolders(const Filepath& folder);

	private:
		std::vector<Filepath> m_roots;
		Filepath m_folder;
		std::vector<Filepath> m_backHistory;
		std::vector<Filepath> m_forwardHistory;

		std::vector<FolderEntry> m_entries;
		FileTileGrid m_grid;
		char m_search[128] = {};
		float m_tileSize = 96.0f;

		// The tree's subfolders, by the relative path of the folder they're in
		std::unordered_map<std::string, std::vector<Filepath>> m_subfolders;
		// Opens the tree down to the current folder on the next draw, e.g. after going there from the tiles
		bool m_revealFolder = true;

		std::chrono::steady_clock::time_point m_lastRefresh;
		// Refreshed at the start of the next draw, e.g. after the search changes
		bool m_refreshRequested = false;

		EventSubscription m_thumbnailWritten;
	};
}
