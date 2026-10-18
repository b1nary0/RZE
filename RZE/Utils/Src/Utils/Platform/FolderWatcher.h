#pragma once

#include <Utils/Platform/Filepath.h>

// Tells whether anything in a folder, or in any folder under it, changed: a file or folder made, renamed or deleted,
// or a file written. Not what changed, just that something did. Polled, so there's no thread behind it.
class FolderWatcher
{
public:
	// Starts watching folder. Watches nothing (IsWatching is false) if the folder doesn't exist.
	explicit FolderWatcher(const Filepath& folder);
	~FolderWatcher();

	FolderWatcher(FolderWatcher&& other) noexcept;
	FolderWatcher& operator=(FolderWatcher&& other) noexcept;
	FolderWatcher(const FolderWatcher&) = delete;
	FolderWatcher& operator=(const FolderWatcher&) = delete;

public:
	bool IsWatching() const { return m_handle != nullptr; }
	const Filepath& GetFolder() const { return m_folder; }

	// Whether anything changed since it was made or last polled. Doesn't wait. The OS tells of a change soon after
	// it's made, not straight away, and many changes in a row may be told as one.
	bool PollChanged();

private:
	void Close();

private:
	Filepath m_folder;
	// The OS's handle to wait on; null when not watching
	void* m_handle = nullptr;
};
