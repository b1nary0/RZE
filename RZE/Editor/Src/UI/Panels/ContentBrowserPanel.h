#pragma once

#include <UI/Framework/EditorEvents.h>
#include <UI/Framework/EditorPanel.h>
#include <UI/Widgets/FileTileGrid.h>

#include <Services/FolderNavigator.h>

#include <Utils/Platform/Filepath.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace Editor
{
	// Browses the project's Assets and ProjectData folders: a folder tree beside a grid of tiles like the file
	// browser's, with back, forward and up (also the mouse's side buttons and Backspace), search, and right-click
	// actions on files and folders. Double-clicking a folder opens it, and a scene loads it. Files can be dragged onto
	// the Object Inspector's asset fields that take them, and meshes into the scene view. Files and folders can be
	// renamed (F2), deleted (Delete) and made (New Folder), each of which can be undone (AssetFileCommands). Names
	// starting with a dot, like the editor's trash, aren't shown. Changes made outside the editor show up as they
	// happen (ProjectFilesChangedEvent).
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
		void DrawDeletePrompt(EditorContext& context);

		// Reads the current folder and the tree's folders again. If the folder is gone, shows the nearest one above it
		// that's still there (FolderNavigator::Refresh).
		void Refresh();
		// Has the listed scenes' and meshes' thumbnails rendered again where they're older than their file, e.g. a mesh
		// written again outside the editor
		void RequestOutdatedThumbnails(EditorContext& context);

		// Opens a folder, or loads a scene; anything else stays selected
		void Open(EditorContext& context, const Filepath& file);
		// Asks about unsaved changes first, like File > Load Scene
		void LoadScene(EditorContext& context, const Filepath& scene);

		// To the name typed into its tile (FileNames::ResolveRename)
		void Rename(EditorContext& context, const FolderEntry& entry, const std::string& name);
		// Asks first (DrawDeletePrompt)
		void RequestDelete(const Filepath& path);
		void Delete(EditorContext& context, const Filepath& path);
		// "New Folder" in the current folder, numbered if taken, then renames it
		void CreateFolder(EditorContext& context);

		// Keeps the folder shown, the history and the selection on what moved, and shows the change straight away,
		// including from undo or redo
		void OnAssetFileChanged(const AssetFileChangedEvent& event);

		// Read the first time a tree node needs them, then kept until Refresh
		const std::vector<Filepath>& GetSubfolders(const Filepath& folder);

	private:
		// The folder shown, its entries, the search and the history
		FolderNavigator m_navigator;
		FileTileGrid m_grid;
		float m_tileSize = 96.0f;

		// The tree's subfolders, by the relative path of the folder they're in
		std::unordered_map<std::string, std::vector<Filepath>> m_subfolders;
		// Opens the tree down to the current folder on the next draw, e.g. after going there from the tiles
		bool m_revealFolder = true;

		// Refreshed at the start of the next draw, e.g. after the search changes or the files change
		bool m_refreshRequested = false;
		// Made at the start of the next draw, outside the menu that asked for it
		bool m_createFolderRequested = false;
		// The folder New Folder just asked for, which starts renaming once it's made; until the next draw
		Filepath m_renameWhenCreated;

		// What's being asked about deleting, while the prompt is up
		struct DeletePrompt;
		std::unique_ptr<DeletePrompt> m_deletePrompt;

		EventSubscription m_thumbnailWritten;
		EventSubscription m_assetFileChanged;
		EventSubscription m_projectFilesChanged;
	};
}
