#include <StdAfx.h>

#include <Utils/Platform/Shell.h>

#include <Utils/Platform/Filepath.h>
#include <Utils/Platform/Linux/SpawnProcessLinux.h>

#include <filesystem>
#include <string>
#include <thread>
#include <vector>

namespace
{
	// A path as a file:// URI. Everything but unreserved characters and '/' is percent-encoded, which also leaves
	// nothing to escape when it's put in quotes.
	std::string ToFileURI(const std::string& absolutePath)
	{
		static const char k_hexDigits[] = "0123456789ABCDEF";

		std::string uri = "file://";
		for (const unsigned char c : absolutePath)
		{
			const bool isUnreserved = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')
				|| c == '-' || c == '.' || c == '_' || c == '~' || c == '/';
			if (isUnreserved)
			{
				uri += static_cast<char>(c);
			}
			else
			{
				uri += '%';
				uri += k_hexDigits[c >> 4];
				uri += k_hexDigits[c & 0xF];
			}
		}
		return uri;
	}

	// Runs a program to the end, with its output discarded. Whether it exited with 0.
	bool RunQuietly(const std::string& program, const std::vector<std::string>& arguments)
	{
		const pid_t pid = Spawn::Start(program, arguments, Spawn::k_discardOutput);
		return pid != -1 && Spawn::Wait(pid) == 0;
	}
}

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

		// The freedesktop.org file manager interface (Dolphin, Nautilus and others) opens a folder, or selects a file
		// in its folder as Explorer's /select does. Without one, the folder is opened with nothing selected.
		const std::string method = isFolder ? "ShowFolders" : "ShowItems";
		const std::string uriList = "['" + ToFileURI(absolute.string()) + "']";
		const std::string folder = isFolder ? absolute.string() : absolute.parent_path().string();

		// On a thread of its own: the call waits on the file manager, which may have to start first
		std::thread([method, uriList, folder]()
			{
				const bool bShown = RunQuietly("gdbus", { "call", "--session",
					"--dest", "org.freedesktop.FileManager1",
					"--object-path", "/org/freedesktop/FileManager1",
					"--method", "org.freedesktop.FileManager1." + method,
					uriList, "" });
				if (!bShown)
				{
					RunQuietly("xdg-open", { folder });
				}
			}).detach();
		return true;
	}

	// What KDE, GNOME and the freedesktop specs call them, whichever file manager it is
	const char* GetFileBrowserName()
	{
		return "File Manager";
	}

	const char* GetRecycleBinName()
	{
		return "Trash";
	}

	bool MoveToRecycleBin(const Filepath& path)
	{
		std::error_code error;
		const std::filesystem::path absolute(path.GetAbsolutePath());
		if (!std::filesystem::exists(absolute, error))
		{
			return false;
		}

		// gio (part of GLib, on GNOME and KDE alike) moves it to the freedesktop.org trash that file managers show
		return RunQuietly("gio", { "trash", absolute.lexically_normal().string() });
	}
}
