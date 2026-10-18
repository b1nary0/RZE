#pragma once

#include <UI/Framework/EditorEvents.h>
#include <UI/Framework/EditorPanel.h>
#include <UI/Widgets/FileTileGrid.h>

#include <Services/FolderListing.h>

#include <Utils/Platform/Filepath.h>

#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace Editor
{
	// Browses the project's Assets and ProjectData folders: a folder tree beside a grid of tiles like the file
	// browser's, with back, forward and up (also the mouse's side buttons and Backspace), search, and right-click
	// actions on files and folders. Double-clicking a folder opens it, and a scene loads it. Meshes can be dragged into
	// the scene view. Files and folders can be renamed (F2), deleted to the Recycle Bin (Delete) and made (New Folder).
	class ContentBrowserPanel : public EditorPanel
	{
	public:
		ContentBrowserPanel();
		~ContentBrowserPanel() override;

		static PanelDesc Describe();

	private:
		void Initialize(EditorContext& context) override;
		void Draw(EditorContext& context) override;

		void DrawToolbar();
		void DrawFolderTree();
		void DrawFolderNode(const Filepath& folder);
		void DrawTiles(EditorContext& context);
		void HandleNavigationInput(EditorContext& context);
		void DrawDeletePrompt();

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

		// To the name typed into its tile (ResolveRename), keeping the history and selection on it
		void Rename(const FolderEntry& entry, const std::string& name);
		// Asks first (DrawDeletePrompt)
		void RequestDelete(const Filepath& path);
		void Delete(const Filepath& path);
		// "New Folder" in the current folder, numbered if taken, then renames it
		void CreateFolder();

		// The history's folders at or inside from, moved to where they are under to
		void MoveHistory(const Filepath& from, const Filepath& to);
		// Drops the history's folders at or inside path
		void ForgetHistory(const Filepath& path);

		// The current folder's entry for path, or null
		const FolderEntry* FindEntry(const Filepath& path) const;

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
		// Made at the start of the next draw, outside the menu that asked for it
		bool m_createFolderRequested = false;

		// What's being asked about deleting, while the prompt is up
		struct DeletePrompt;
		std::unique_ptr<DeletePrompt> m_deletePrompt;

		EventSubscription m_thumbnailWritten;
	};
}
