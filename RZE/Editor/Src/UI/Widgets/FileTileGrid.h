#pragma once

#include <Services/FolderListing.h>

#include <EngineCore/Resources/ResourceHandler.h>

#include <Utils/Platform/Filepath.h>

#include <ImGui/imgui.h>

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class AsyncOperationGroup;
class ResourcePreloadOperation;
class Texture2D;

namespace Editor
{
	// The ImGui drag and drop payload a dragged file tile carries: the file's relative path, null-terminated
	constexpr const char* k_assetPathPayload = "RZE_ASSET_PATH";

	// An entry in a tile's right-click menu, on files and folders alike
	struct FileTileAction
	{
		std::string Label;
		std::function<void(const Filepath& file)> Run;
		// Shown only for the files this returns true for; every file if it's empty
		std::function<bool(const Filepath& file)> AppliesTo;
		// Shown beside the label, e.g. "F2"; the grid's owner handles the key
		std::string Shortcut;
		// Shown when the item is hovered
		std::string Tooltip;
		// Why it can't be used on a file (shown when the greyed-out item is hovered), or empty if it can
		std::function<std::string(const Filepath& file)> WhyDisabled;
	};

	// What a FileTileGrid's tiles show, and what they offer
	struct FileTileGridOptions
	{
		// The image a file's tile shows, e.g. its thumbnail. No function, or an invalid or missing file, shows a
		// placeholder with the file's extension.
		std::function<Filepath(const Filepath& file)> ThumbnailFor;
		// Called once a tile is shown whose image (ThumbnailFor) doesn't exist yet, once per file until the images are
		// released, e.g. to have it rendered (RefreshThumbnail shows it once it's written)
		std::function<void(const Filepath& file)> OnThumbnailMissing;
		// The right-click menu on a file's tile
		std::vector<FileTileAction> TileActions;
		// The right-click menu on the grid's empty space: its items, drawn inside the popup. None if it's empty.
		std::function<void()> DrawBackgroundMenu;
		// Whether a file's tile can be dragged (as a k_assetPathPayload) to a drop target; none can if it's empty
		std::function<bool(const Filepath& file)> CanDrag;

		// Tiles can be renamed in place (BeginRename) when both are set. The name typed is a file's stem, or a
		// folder's whole name. ValidateName says what's wrong with a name, shown under it, or returns empty if it's
		// fine. Rename is called on Enter with a name ValidateName accepts, and only if it changed.
		std::function<std::string(const FolderEntry& entry, const std::string& name)> ValidateName;
		std::function<void(const FolderEntry& entry, const std::string& name)> Rename;
	};

	// What happened in a FileTileGrid this frame
	struct FileTileGridResult
	{
		// The tile clicked, which is now the selection; invalid if none was
		Filepath Clicked;
		// The tile double-clicked; invalid if none was
		Filepath Activated;
	};

	// A scrolling grid of large tiles, one per folder entry, each showing the file's image (loaded in the background) or
	// a placeholder, its name and how long ago it changed. Keeps the selection, which the arrow keys move while its
	// window is focused. FileBrowserModal and the Content Browser panel each own one.
	class FileTileGrid
	{
	public:
		FileTileGrid();
		~FileTileGrid();

		FileTileGrid(const FileTileGrid&) = delete;
		FileTileGrid& operator=(const FileTileGrid&) = delete;

	public:
		void SetOptions(FileTileGridOptions options);

		// Draws entries in a child window of size (as for ImGui::BeginChild), with tiles tileSize pixels across, or
		// emptyText in the middle if there are none
		FileTileGridResult Draw(const std::vector<FolderEntry>& entries, float tileSize, const ImVec2& size, const char* emptyText);

		const Filepath& GetSelected() const { return m_selected; }
		// scrollTo brings its tile into view on the next Draw
		void SetSelected(const Filepath& file, bool scrollTo);
		// On the next Draw
		void ScrollToTop() { m_scrollToTop = true; }

		// Turns file's tile name into a text box, selected and scrolled to, until Enter renames it (with the options'
		// Rename) or Escape or clicking elsewhere cancels. Stops if the file is no longer listed.
		void BeginRename(const Filepath& file);
		bool IsRenaming() const { return m_rename != nullptr; }

		// Reloads the tiles that show image, e.g. once a thumbnail has been rendered again
		void RefreshThumbnail(const Filepath& image);
		// Stops the loads still running and drops the images
		void ReleaseThumbnails();

	private:
		// Adds a click or double-click on it to result
		void DrawTile(const FolderEntry& entry, int index, const ImVec2& size, FileTileGridResult& result);
		void DrawTileImage(const FolderEntry& entry, const ImVec2& min, const ImVec2& max);
		// The text box over the tile's name, between left and right with the name's top at y
		void DrawRenameBox(const FolderEntry& entry, float left, float right, float y);
		bool CanRename() const { return m_options.ValidateName && m_options.Rename; }
		// Whether file's tile is the one being renamed
		bool IsRenaming(const Filepath& file) const;
		// Runs after the tiles are drawn, since it can change what the grid's owner lists
		void QueueAction(const std::function<void(const Filepath&)>& run, const Filepath& file);
		void HandleArrowKeys(const std::vector<FolderEntry>& entries, int columns);
		void DrawBackgroundMenu();
		bool HasActionsFor(const Filepath& file) const;

		// Null while it's loading, or if there's no image. Starts loading it the first time it's asked for.
		const Texture2D* GetThumbnail(const Filepath& file);

		int FindSelectedIndex(const std::vector<FolderEntry>& entries) const;

	private:
		FileTileGridOptions m_options;

		Filepath m_selected;
		bool m_scrollToTop = false;
		bool m_scrollToSelected = false;

		// An image a tile shows, loaded in the background the first time a tile shows it
		struct Thumbnail
		{
			// Empty while it loads, and for one that doesn't exist or failed to load
			ResourceHandle Texture;
			// The load, until it's finished
			std::shared_ptr<ResourcePreloadOperation> Load;
		};
		// The images shown so far, by relative path
		std::unordered_map<std::string, Thumbnail> m_thumbnails;
		// The images loading, reported as one task ("Loading thumbnails"); another starts if more load once it's finished
		std::shared_ptr<AsyncOperationGroup> m_thumbnailLoads;

		// The tile being renamed and the name typed so far; only while a rename is under way
		struct TileRename;
		std::unique_ptr<TileRename> m_rename;

		// A tile action picked this frame, until it runs once the tiles are drawn
		struct PendingTileAction;
		std::unique_ptr<PendingTileAction> m_pendingAction;
	};

	// The breadcrumb from root down to folder: a button for each folder above, then folder's name. Returns the folder
	// whose button was clicked, or an invalid path.
	Filepath DrawBreadcrumb(const Filepath& root, const Filepath& folder);
}
