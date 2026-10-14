#pragma once

#include <Services/FolderListing.h>

#include <EngineCore/Resources/ResourceHandler.h>

#include <Utils/Platform/Filepath.h>

#include <ImGui/imgui.h>

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

class Texture2D;

namespace Editor
{
	// An entry in a tile's right-click menu
	struct FileBrowserTileAction
	{
		std::string Label;
		std::function<void(const Filepath& file)> Run;
	};

	enum class EFileBrowserMode
	{
		// Picks an existing file
		Open,
		// Picks a file to write: a typed name, or an existing file to replace (after asking)
		Save
	};

	// What FileBrowserModal::Open shows, and where the choice goes
	struct FileBrowserRequest
	{
		EFileBrowserMode Mode = EFileBrowserMode::Open;
		std::string Title;
		std::string ConfirmLabel = "Open";
		// Browsing starts here and can't leave it
		Filepath RootFolder;
		// Files with one of these extensions are listed, e.g. ".scene"; empty lists every file
		std::vector<std::string> Extensions;
		// Selected when the browser opens, if it's in RootFolder. In Save mode it also names the file to save.
		Filepath InitialSelection;
		// Save mode: the name the file name field starts with when there's no InitialSelection. The first of Extensions
		// is added to a typed name that doesn't end with it.
		std::string DefaultFileName = "Untitled";

		// The image a file's tile shows, e.g. its thumbnail. No function, or an invalid or missing file, shows a
		// placeholder with the file's extension.
		std::function<Filepath(const Filepath& file)> ThumbnailFor;
		// Called once a tile is shown whose image (ThumbnailFor) doesn't exist yet, once per file each time the browser
		// opens, e.g. to have it rendered (RefreshThumbnail shows it once it's written)
		std::function<void(const Filepath& file)> OnThumbnailMissing;
		// The right-click menu on a file's tile
		std::vector<FileBrowserTileAction> TileActions;

		// The choice comes back through one of these: OnChosen is called with it, or, for an immediate-mode widget
		// that has nowhere to keep a callback's result, ConsumeResult hands it to the widget whose ID is RequesterID.
		std::function<void(const Filepath& file)> OnChosen;
		ImGuiID RequesterID = 0;
		// When the browser closes without a choice, including when another request replaces this one
		std::function<void()> OnCancelled;
	};

	// A modal file browser: a grid of large tiles showing each file's thumbnail, with search and subfolders. Owned by
	// EditorContext; EditorApp displays it each frame.
	class FileBrowserModal
	{
	public:
		FileBrowserModal() = default;
		~FileBrowserModal() = default;

		FileBrowserModal(const FileBrowserModal&) = delete;
		FileBrowserModal& operator=(const FileBrowserModal&) = delete;

	public:
		// Opens the browser for request. A request it's already showing is replaced, and cancelled.
		void Open(FileBrowserRequest request);
		bool IsOpen() const { return m_isOpen || m_shouldOpen; }

		// True once, with the chosen file, after the request from the widget with this ID was answered. Call it from
		// the widget each frame.
		bool ConsumeResult(ImGuiID requesterID, Filepath& outFile);

		// Reloads the tiles that show image, e.g. once a thumbnail has been rendered again
		void RefreshThumbnail(const Filepath& image);

		// Must be called at the root of the ImGui ID stack (outside any Begin/End pair)
		void Display();

	private:
		void DrawHeader();
		void DrawBreadcrumb();
		void DrawTiles();
		// True if it was double-clicked
		bool DrawTile(const FolderEntry& entry, int index, const ImVec2& size);
		void DrawTileImage(const FolderEntry& entry, const ImVec2& min, const ImVec2& max);
		void DrawFooter();
		void HandleKeys(int columns);

		void Navigate(const Filepath& folder);
		void Refresh();
		// Opens the selected folder, or chooses the selected file (Open mode) or the typed name (Save mode)
		void ConfirmSelection();
		// Save mode: saves to the typed name, asking first if that replaces a file
		void ConfirmSave();
		// By value: closing clears the entries a selected path would point into
		void Choose(Filepath file);
		void Cancel();
		// Closes without answering the request
		void Close();
		void SetFileName(const std::string& name);
		bool IsSaving() const { return m_request.Mode == EFileBrowserMode::Save; }

		// Null while it's still to be loaded, or if there's no image
		const Texture2D* GetThumbnail(const Filepath& file);
		void ReleaseThumbnails();

		const FolderEntry* GetSelected() const;

	private:
		FileBrowserRequest m_request;
		bool m_shouldOpen = false;
		bool m_isOpen = false;

		Filepath m_folder;
		std::vector<FolderEntry> m_entries;
		Filepath m_selected;
		char m_search[128] = {};
		// Escape leaves a text field rather than closing the browser
		bool m_isTextActive = false;

		// Save mode
		char m_fileName[128] = {};
		bool m_focusFileName = false;
		// Why the typed name can't be saved to, shown over the footer
		std::string m_saveError;
		// The existing file the typed name would replace, while the footer asks about it
		Filepath m_pendingReplace;
		float m_tileSize = 138.0f;
		bool m_scrollToTop = false;
		bool m_scrollToSelected = false;

		// The images loaded so far, by relative path; an empty handle for one that doesn't exist
		std::unordered_map<std::string, ResourceHandle> m_thumbnails;
		int m_loadsThisFrame = 0;

		// A tile action picked this frame, run once the tiles are drawn
		std::function<void(const Filepath&)> m_pendingAction;
		Filepath m_pendingActionFile;

		// The last answer to a RequesterID request, waiting for ConsumeResult
		ImGuiID m_resultRequester = 0;
		Filepath m_result;
	};
}
