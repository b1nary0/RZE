#include <StdAfx.h>

#include <Utils/Platform/FolderWatcher.h>

#include <Utils/DebugUtils/Debug.h>

#include <cerrno>
#include <cstring>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include <sys/inotify.h>
#include <unistd.h>

namespace
{
	// Made, renamed or deleted files and folders, and files written
	constexpr uint32_t k_changes = IN_CREATE | IN_DELETE | IN_MOVED_FROM | IN_MOVED_TO | IN_MODIFY;

	// inotify watches a folder but not the folders under it, so each folder in the tree has a watch of its own
	struct WatchedTree
	{
		int inotify = -1;
		int rootWatch = -1;
		// Each watch's folder, so a folder made in it can be watched too
		std::unordered_map<int, std::string> folders;
		bool bHasWarned = false;
	};

	// Watches folder and every folder under it, and returns folder's watch (-1 if it can't be watched). Folders
	// already watched keep their watch, under their current path. A folder under it that can't be watched is warned of
	// once; the root's own failure is the caller's to tell.
	int WatchFolders(WatchedTree& tree, const std::string& folder, bool bIsRoot = false)
	{
		const int watch = inotify_add_watch(tree.inotify, folder.c_str(), k_changes | IN_ONLYDIR);
		if (watch < 0)
		{
			// Gone already, or not a folder, is nothing to warn about
			if (!bIsRoot && errno != ENOENT && errno != ENOTDIR && !tree.bHasWarned)
			{
				tree.bHasWarned = true;
				RZE_LOG_WARNING("Can't watch " << folder << " for changes (" << std::strerror(errno) << "); changes in folders like it may be missed.");
			}
			return -1;
		}
		tree.folders[watch] = folder;

		std::error_code error;
		for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(folder, error))
		{
			if (entry.is_directory(error) && !entry.is_symlink(error))
			{
				WatchFolders(tree, entry.path().string());
			}
		}
		return watch;
	}
}

FolderWatcher::FolderWatcher(const Filepath& folder)
	: m_folder(folder)
{
	// Read without waiting, and not passed on to programs started from here
	const int inotify = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
	if (inotify < 0)
	{
		if (errno == EMFILE)
		{
			RZE_LOG_WARNING("No inotify instances left (fs.inotify.max_user_instances) to watch " << folder.GetRelativePath() << ".");
		}
		return;
	}

	std::unique_ptr<WatchedTree> tree = std::make_unique<WatchedTree>();
	tree->inotify = inotify;
	tree->rootWatch = WatchFolders(*tree, folder.GetAbsolutePath(), true);
	if (tree->rootWatch == -1)
	{
		close(inotify);
		return;
	}
	m_handle = tree.release();
}

FolderWatcher::~FolderWatcher()
{
	Close();
}

FolderWatcher::FolderWatcher(FolderWatcher&& other) noexcept
	: m_folder(std::move(other.m_folder))
	, m_handle(std::exchange(other.m_handle, nullptr))
{
}

FolderWatcher& FolderWatcher::operator=(FolderWatcher&& other) noexcept
{
	if (this != &other)
	{
		Close();
		m_folder = std::move(other.m_folder);
		m_handle = std::exchange(other.m_handle, nullptr);
	}
	return *this;
}

bool FolderWatcher::PollChanged()
{
	if (m_handle == nullptr)
	{
		return false;
	}
	WatchedTree& tree = *static_cast<WatchedTree*>(m_handle);

	bool bChanged = false;
	bool bRootGone = false;
	alignas(inotify_event) char buffer[4096];
	for (;;)
	{
		const ssize_t length = read(tree.inotify, buffer, sizeof(buffer));
		if (length < 0 && errno == EINTR)
		{
			continue;
		}
		// Nothing more for now
		if (length <= 0)
		{
			break;
		}

		for (ssize_t offset = 0; offset < length;)
		{
			const inotify_event& event = *reinterpret_cast<const inotify_event*>(buffer + offset);
			offset += static_cast<ssize_t>(sizeof(inotify_event) + event.len);

			bChanged = true;
			if (event.mask & IN_IGNORED)
			{
				// Its folder went, e.g. deleted
				bRootGone = bRootGone || event.wd == tree.rootWatch;
				tree.folders.erase(event.wd);
			}
			else if (event.mask & IN_Q_OVERFLOW)
			{
				// Events were dropped, maybe a folder being made: watch anything not yet watched
				const std::unordered_map<int, std::string>::const_iterator root = tree.folders.find(tree.rootWatch);
				if (root != tree.folders.end())
				{
					const std::string rootFolder = root->second;
					WatchFolders(tree, rootFolder);
				}
			}
			else if ((event.mask & IN_ISDIR) && (event.mask & (IN_CREATE | IN_MOVED_TO)) && event.len > 0)
			{
				const std::unordered_map<int, std::string>::const_iterator parent = tree.folders.find(event.wd);
				if (parent != tree.folders.end())
				{
					const std::string folder = parent->second + "/" + event.name;
					WatchFolders(tree, folder);
				}
			}
		}
	}

	if (bRootGone)
	{
		// Nothing more will come
		Close();
	}
	return bChanged;
}

void FolderWatcher::Close()
{
	if (m_handle != nullptr)
	{
		WatchedTree* tree = static_cast<WatchedTree*>(m_handle);
		close(tree->inotify);
		delete tree;
		m_handle = nullptr;
	}
}
