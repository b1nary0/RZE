#include <Services/FolderNavigator.h>

#include <Utils/DebugUtils/Debug.h>

#include <algorithm>

namespace Editor
{
	void FolderNavigator::Reset(std::vector<Filepath> roots, std::vector<std::string> extensions, const Filepath& startFolder)
	{
		AssertExpr(!roots.empty());
		m_roots = std::move(roots);
		m_extensions = std::move(extensions);
		m_backHistory.clear();
		m_forwardHistory.clear();

		const bool isStartInRoots = startFolder.IsValid()
			&& std::any_of(m_roots.begin(), m_roots.end(), [&startFolder](const Filepath& root) { return startFolder.IsInside(root); });
		ShowFolder(isStartInRoots ? startFolder : m_roots.front());
	}

	const FolderEntry* FolderNavigator::FindEntry(const Filepath& path) const
	{
		const auto entry = std::find_if(m_entries.begin(), m_entries.end(),
			[&path](const FolderEntry& candidate) { return candidate.Path == path; });
		return entry != m_entries.end() ? &*entry : nullptr;
	}

	const Filepath& FolderNavigator::GetRoot() const
	{
		const auto root = std::find_if(m_roots.begin(), m_roots.end(),
			[this](const Filepath& candidate) { return m_folder == candidate || m_folder.IsInside(candidate); });
		return root != m_roots.end() ? *root : m_roots.front();
	}

	bool FolderNavigator::IsRoot(const Filepath& folder) const
	{
		return std::find(m_roots.begin(), m_roots.end(), folder) != m_roots.end();
	}

	void FolderNavigator::Navigate(const Filepath& folder)
	{
		if (folder == m_folder)
		{
			return;
		}

		m_backHistory.push_back(m_folder);
		m_forwardHistory.clear();
		ShowFolder(folder);
	}

	void FolderNavigator::GoBack()
	{
		if (m_backHistory.empty())
		{
			return;
		}

		m_forwardHistory.push_back(m_folder);
		const Filepath folder = m_backHistory.back();
		m_backHistory.pop_back();
		ShowFolder(folder);
	}

	void FolderNavigator::GoForward()
	{
		if (m_forwardHistory.empty())
		{
			return;
		}

		m_backHistory.push_back(m_folder);
		const Filepath folder = m_forwardHistory.back();
		m_forwardHistory.pop_back();
		ShowFolder(folder);
	}

	Filepath FolderNavigator::GoUp()
	{
		if (IsRoot(m_folder))
		{
			return Filepath();
		}

		const Filepath child = m_folder;
		Navigate(m_folder.GetParentFolder());
		return child;
	}

	void FolderNavigator::Refresh()
	{
		if (!m_folder.Exists())
		{
			// The highest folder that went, which took the current one with it
			Filepath gone = m_folder;
			Filepath folder = m_folder;
			while (!IsRoot(folder) && !folder.Exists())
			{
				gone = folder;
				folder = folder.GetParentFolder();
			}
			ShowFolder(folder);
			ForgetHistory(gone);
			return;
		}

		m_entries = ListFolder(m_folder, m_extensions, m_search);
	}

	void FolderNavigator::OnPathChanged(const Filepath& from, const Filepath& to)
	{
		const bool isShownFolderChanged = m_folder == from || m_folder.IsInside(from);
		if (to.IsValid())
		{
			MoveHistory(from, to);
			if (isShownFolderChanged)
			{
				ShowFolder(FollowRename(m_folder, from, to));
			}
		}
		else
		{
			if (isShownFolderChanged)
			{
				ShowFolder(from.GetParentFolder());
			}
			ForgetHistory(from);
		}
	}

	void FolderNavigator::ShowFolder(const Filepath& folder)
	{
		m_folder = folder;
		ClearSearch();
		m_entries = ListFolder(m_folder, m_extensions, m_search);

		if (m_onFolderShown)
		{
			m_onFolderShown();
		}
	}

	void FolderNavigator::MoveHistory(const Filepath& from, const Filepath& to)
	{
		const auto move = [&from, &to](Filepath& folder) { folder = FollowRename(folder, from, to); };
		std::for_each(m_backHistory.begin(), m_backHistory.end(), move);
		std::for_each(m_forwardHistory.begin(), m_forwardHistory.end(), move);
	}

	void FolderNavigator::ForgetHistory(const Filepath& path)
	{
		const auto isGone = [&path](const Filepath& folder) { return folder == path || folder.IsInside(path); };
		const auto forget = [this, &isGone](std::vector<Filepath>& history)
			{
				history.erase(std::remove_if(history.begin(), history.end(), isGone), history.end());
				// With what was between them gone, folders beside each other can be the same, or the one shown, so
				// going back would seem to do nothing
				history.erase(std::unique(history.begin(), history.end()), history.end());
				if (!history.empty() && history.back() == m_folder)
				{
					history.pop_back();
				}
			};
		forget(m_backHistory);
		forget(m_forwardHistory);
	}

	Filepath FollowRename(const Filepath& path, const Filepath& from, const Filepath& to)
	{
		if (path != from && !path.IsInside(from))
		{
			return path;
		}
		return Filepath(to.GetRelativePath() + path.GetRelativePath().substr(from.GetRelativePath().size()));
	}
}
