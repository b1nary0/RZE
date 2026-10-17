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

class ResourcePreloadOperation;
class Texture2D;

namespace Editor
{
	// The ImGui drag and drop payload a dragged file tile carries: the file's relative path, null-terminated
	constexpr const char* k_assetPathPayload = "RZE_ASSET_PATH";

	// An entry in a file tile's right-click menu
	struct FileTileAction
	{
		std::string Label;
		std::function<void(const Filepath& file)> Run;
		// Shown only for the files this returns true for; every file if it's empty
		std::function<bool(const Filepath& file)> AppliesTo;
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
		FileTileGrid() = default;
		~FileTileGrid() = default;

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

		// Reloads the tiles that show image, e.g. once a thumbnail has been rendered again
		void RefreshThumbnail(const Filepath& image);
		// Stops the loads still running and drops the images
		void ReleaseThumbnails();

	private:
		// Adds a click or double-click on it to result
		void DrawTile(const FolderEntry& entry, int index, const ImVec2& size, FileTileGridResult& result);
		void DrawTileImage(const FolderEntry& entry, const ImVec2& min, const ImVec2& max);
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

		// A tile action picked this frame, run once the tiles are drawn
		std::function<void(const Filepath&)> m_pendingAction;
		Filepath m_pendingActionFile;
	};

	// The breadcrumb from root down to folder: a button for each folder above, then folder's name. Returns the folder
	// whose button was clicked, or an invalid path.
	Filepath DrawBreadcrumb(const Filepath& root, const Filepath& folder);
}
