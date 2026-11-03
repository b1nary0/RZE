#include <StdAfx.h>

#include <Utils/Platform/Shell.h>

#include <Utils/Platform/Filepath.h>

#include <filesystem>
#include <string>

#include <Windows.h>
#include <shellapi.h>

namespace Shell
{
	bool ShowInFileBrowser(const Filepath& path)
	{
		std::error_code error;
		const std::filesystem::path absolute(path.GetAbsolutePath());
		const bool isFolder = std::filesystem::is_directory(absolute, error);
		if (!isFolder && !std::filesystem::is_regular_file(absolute, error))
		{
			return false;
		}

		// Explorer selects a file given "/select,<path>", and opens a folder given just its path
		const std::wstring arguments = (isFolder ? L"\"" : L"/select,\"") + absolute.wstring() + L"\"";
		const HINSTANCE result = ShellExecuteW(nullptr, L"open", L"explorer.exe", arguments.c_str(), nullptr, SW_SHOWNORMAL);
		// Anything above 32 is success
		return reinterpret_cast<INT_PTR>(result) > 32;
	}

	bool MoveToRecycleBin(const Filepath& path)
	{
		std::error_code error;
		const std::filesystem::path absolute(path.GetAbsolutePath());
		if (!std::filesystem::exists(absolute, error))
		{
			return false;
		}

		// The shell takes a list of paths, each ending in a null, with one more null after the last
		const std::wstring from = absolute.lexically_normal().wstring() + L'\0';

		SHFILEOPSTRUCTW operation = {};
		operation.wFunc = FO_DELETE;
		operation.pFrom = from.c_str();
		// Allowing undo is what sends it to the Recycle Bin rather than deleting it
		operation.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT;
		return SHFileOperationW(&operation) == 0 && !operation.fAnyOperationsAborted;
	}
}
