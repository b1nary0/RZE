#pragma once

#include <Services/FolderListing.h>

#include <Utils/Platform/Filepath.h>

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

namespace Editor
{
	// Which folder a browser shows and what's in it: the folder, its entries (ListFolder), a search over them, and the
	// back and forward history, kept within its roots. It follows renames and deletes of the folders it knows, and falls
	// back to the nearest folder still there when the one shown goes. No ImGui; the browsers (ContentBrowserPanel,
	// FileBrowserModal) draw from it. Main thread only.
	class FolderNavigator
	{
	public:
		// Starts over in startFolder (the first root if it isn't inside one), with no history or search. roots are where
		// browsing can go, at least one; extensions say which files are listed (any file if empty), like ListFolder.
		void Reset(std::vector<Filepath> roots, std::vector<std::string> extensions, const Filepath& startFolder = Filepath());
		// Called after the folder shown changes, however it changed, e.g. so a browser drops its old tiles' images
		void SetOnFolderShown(std::function<void()> onFolderShown) { m_onFolderShown = std::move(onFolderShown); }

		const Filepath& GetFolder() const { return m_folder; }
		const std::vector<FolderEntry>& GetEntries() const { return m_entries; }
		// The folder shown's entry for path, or null
		const FolderEntry* FindEntry(const Filepath& path) const;
		const std::vector<Filepath>& GetRoots() const { return m_roots; }
		// The root the folder shown is in
		const Filepath& GetRoot() const;
		bool IsRoot(const Filepath& folder) const;

		// Goes to folder, adding where it was to the back history and clearing forward; the folder shown does nothing
		void Navigate(const Filepath& folder);
		bool CanGoBack() const { return !m_backHistory.empty(); }
		void GoBack();
		bool CanGoForward() const { return !m_forwardHistory.empty(); }
		void GoForward();
		bool CanGoUp() const { return !IsRoot(m_folder); }
		// Goes to the folder above, like Navigate. Returns the folder it came out of, e.g. to select it; invalid at a root.
		Filepath GoUp();

		// The search, as a buffer for a text field to edit in place. Going to another folder clears it; Refresh applies it.
		char* GetSearchBuffer() { return m_search; }
		size_t GetSearchBufferSize() const { return sizeof(m_search); }
		bool HasSearch() const { return m_search[0] != '\0'; }
		void ClearSearch() { m_search[0] = '\0'; }

		// Reads the folder again. If it's gone, shows the nearest folder above it that's still there and forgets the gone
		// ones from the history.
		void Refresh();
		// A file or folder moved (to valid) or was deleted (to invalid): the folder shown and the history follow it, or
		// forget it. Doesn't read the folder again; Refresh does.
		void OnPathChanged(const Filepath& from, const Filepath& to);

	private:
		// Shows folder without touching the history
		void ShowFolder(const Filepath& folder);
		// The history's folders at or inside from, moved to where they are under to
		void MoveHistory(const Filepath& from, const Filepath& to);
		// Drops the history's folders at or inside path, and then any that repeat the one beside them or the one shown.
		// After showing a folder that's still there.
		void ForgetHistory(const Filepath& path);

	private:
		std::vector<Filepath> m_roots;
		std::vector<std::string> m_extensions;
		Filepath m_folder;
		std::vector<FolderEntry> m_entries;
		char m_search[128] = {};

		std::vector<Filepath> m_backHistory;
		std::vector<Filepath> m_forwardHistory;

		std::function<void()> m_onFolderShown;
	};

	// Where path is once from is renamed to to: moved with it if it's from or inside it, otherwise unchanged
	Filepath FollowRename(const Filepath& path, const Filepath& from, const Filepath& to);
}
