#include <StdAfx.h>

#include <Utils/Platform/FolderWatcher.h>

#include <filesystem>
#include <utility>

#include <Windows.h>

namespace
{
	// Made, renamed or deleted files and folders, and files written
	constexpr DWORD k_changes = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE;
}

FolderWatcher::FolderWatcher(const Filepath& folder)
	: m_folder(folder)
{
	const std::wstring path = std::filesystem::path(folder.GetAbsolutePath()).wstring();
	const HANDLE handle = FindFirstChangeNotificationW(path.c_str(), TRUE, k_changes);
	if (handle != INVALID_HANDLE_VALUE)
	{
		m_handle = handle;
	}
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
	if (m_handle == nullptr || WaitForSingleObject(m_handle, 0) != WAIT_OBJECT_0)
	{
		return false;
	}

	// Signalled until it's asked to watch for the next change
	if (!FindNextChangeNotification(m_handle))
	{
		// The folder went, e.g. deleted: nothing more will come
		Close();
	}
	return true;
}

void FolderWatcher::Close()
{
	if (m_handle != nullptr)
	{
		FindCloseChangeNotification(m_handle);
		m_handle = nullptr;
	}
}
