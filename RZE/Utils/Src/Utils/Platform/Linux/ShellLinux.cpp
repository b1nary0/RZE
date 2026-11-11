#include <Utils/Platform/Shell.h>

#include <Utils/DebugUtils/Debug.h>
#include <Utils/Platform/Filepath.h>
#include <Utils/Platform/Linux/SpawnProcessLinux.h>

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <string>
#include <thread>
#include <vector>

#include <fcntl.h>
#include <pwd.h>
#include <sys/stat.h>
#include <unistd.h>

namespace
{
	// Everything but unreserved characters and '/' percent-encoded, as URIs and .trashinfo paths are. That also leaves
	// nothing to escape when it's put in quotes.
	std::string PercentEncode(const std::string& path)
	{
		static const char k_hexDigits[] = "0123456789ABCDEF";

		std::string encoded;
		for (const unsigned char c : path)
		{
			const bool isUnreserved = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')
				|| c == '-' || c == '.' || c == '_' || c == '~' || c == '/';
			if (isUnreserved)
			{
				encoded += static_cast<char>(c);
			}
			else
			{
				encoded += '%';
				encoded += k_hexDigits[c >> 4];
				encoded += k_hexDigits[c & 0xF];
			}
		}
		return encoded;
	}

	std::string ToFileURI(const std::string& absolutePath)
	{
		return "file://" + PercentEncode(absolutePath);
	}

	// Runs a program to the end, with its output discarded. Whether it exited with 0.
	bool RunQuietly(const std::string& program, const std::vector<std::string>& arguments)
	{
		const pid_t pid = Spawn::Start(program, arguments, Spawn::k_discardOutput);
		return pid != -1 && Spawn::Wait(pid) == 0;
	}

	// The Trash is the freedesktop.org Trash spec's, which the file managers of every desktop read: in a trash folder,
	// files/ holds what was trashed, and info/ a <name>.trashinfo for each, saying where it was and when.
	// https://specifications.freedesktop.org/trash-spec/latest/
	struct TrashFolder
	{
		std::filesystem::path Root;
		// The top of the filesystem, which the .trashinfo paths of a trash on another drive are relative to. Empty for
		// the home trash, whose paths are absolute.
		std::filesystem::path PathsFrom;
	};

	bool FailToTrash(const std::filesystem::path& item, const std::string& why)
	{
		RZE_LOG_WARNING_ARGS("Couldn't move %s to the Trash: %s.", item.c_str(), why.c_str());
		return false;
	}

	std::string DescribeError(const char* what)
	{
		return std::string(what) + " (" + std::strerror(errno) + ")";
	}

	// The device path is on, following links. False if it can't be read.
	bool GetDevice(const std::filesystem::path& path, dev_t& outDevice)
	{
		struct stat info;
		if (::stat(path.c_str(), &info) != 0)
		{
			return false;
		}
		outDevice = info.st_dev;
		return true;
	}

	// The device of path, or of the nearest folder above it that exists, which is where path would be made
	bool GetDeviceOfNearest(std::filesystem::path path, dev_t& outDevice)
	{
		for (; !GetDevice(path, outDevice); path = path.parent_path())
		{
			if (path == path.parent_path())
			{
				return false;
			}
		}
		return true;
	}

	// Makes a folder only this user can open, unless it's there. False if it isn't a folder afterwards, or, when
	// mustBeOwn (a trash in a folder other users can write to), it's a link or someone else's.
	bool MakeTrashSubfolder(const std::filesystem::path& folder, bool mustBeOwn)
	{
		if (::mkdir(folder.c_str(), 0700) != 0 && errno != EEXIST)
		{
			return false;
		}

		struct stat info;
		if (!mustBeOwn)
		{
			// The home trash may be a link, e.g. to another drive
			return ::stat(folder.c_str(), &info) == 0 && S_ISDIR(info.st_mode);
		}
		return ::lstat(folder.c_str(), &info) == 0 && S_ISDIR(info.st_mode) && info.st_uid == ::getuid();
	}

	// $XDG_DATA_HOME/Trash, or ~/.local/share/Trash. Empty if there's no home folder.
	std::filesystem::path GetHomeTrash()
	{
		// The XDG base directory spec ignores a relative value
		const char* dataHome = std::getenv("XDG_DATA_HOME");
		if (dataHome != nullptr && dataHome[0] == '/')
		{
			return std::filesystem::path(dataHome) / "Trash";
		}

		const char* home = std::getenv("HOME");
		if (home == nullptr || home[0] != '/')
		{
			const passwd* user = ::getpwuid(::getuid());
			home = user != nullptr ? user->pw_dir : nullptr;
		}
		return home != nullptr ? std::filesystem::path(home) / ".local" / "share" / "Trash" : std::filesystem::path();
	}

	// The top of the filesystem folder is on: the highest folder above it on the same device. folder has no links in it,
	// so these are the real folders above it.
	std::filesystem::path FindTopFolder(const std::filesystem::path& folder, dev_t device)
	{
		std::filesystem::path top = folder;
		dev_t parentDevice = 0;
		while (top != top.parent_path() && GetDevice(top.parent_path(), parentDevice) && parentDevice == device)
		{
			top = top.parent_path();
		}
		return top;
	}

	// The trash for a file on a drive the home trash isn't on, since moving it there would mean copying it:
	// $topdir/.Trash/$uid when an administrator made a shared $topdir/.Trash (a real folder with the sticky bit, so users
	// can't remove each other's), or else $topdir/.Trash-$uid. Empty if neither can be used.
	std::filesystem::path FindTopFolderTrash(const std::filesystem::path& top)
	{
		const std::string uid = std::to_string(::getuid());

		const std::filesystem::path shared = top / ".Trash";
		struct stat info;
		if (::lstat(shared.c_str(), &info) == 0 && S_ISDIR(info.st_mode) && (info.st_mode & S_ISVTX) != 0)
		{
			const std::filesystem::path own = shared / uid;
			if (MakeTrashSubfolder(own, true))
			{
				return own;
			}
		}

		const std::filesystem::path own = top / (".Trash-" + uid);
		return MakeTrashSubfolder(own, true) ? own : std::filesystem::path();
	}

	// The trash item goes to: the home trash if it's on the same drive, or else its own drive's. Its files/ and info/
	// are made if they aren't there. Root is empty if there's none that can be used.
	TrashFolder FindTrashFor(const std::filesystem::path& folder, dev_t device)
	{
		TrashFolder trash;

		const std::filesystem::path home = GetHomeTrash();
		dev_t homeDevice = 0;
		std::error_code error;
		if (!home.empty() && GetDeviceOfNearest(home, homeDevice) && homeDevice == device)
		{
			// ~/.local/share may not be there yet, on a new account
			std::filesystem::create_directories(home.parent_path(), error);
			if (MakeTrashSubfolder(home, false))
			{
				trash.Root = home;
			}
		}
		else
		{
			trash.PathsFrom = FindTopFolder(folder, device);
			trash.Root = FindTopFolderTrash(trash.PathsFrom);
		}

		const bool mustBeOwn = !trash.PathsFrom.empty();
		if (trash.Root.empty() || !MakeTrashSubfolder(trash.Root / "files", mustBeOwn) || !MakeTrashSubfolder(trash.Root / "info", mustBeOwn))
		{
			trash.Root.clear();
		}
		return trash;
	}

	// What a .trashinfo holds: where item was, and the local time, which is what the spec asks for
	std::string MakeTrashInfo(const std::filesystem::path& item, const std::filesystem::path& pathsFrom)
	{
		const std::filesystem::path original = pathsFrom.empty() ? item : item.lexically_relative(pathsFrom);

		const std::time_t now = std::time(nullptr);
		std::tm local = {};
		::localtime_r(&now, &local);
		char date[32];
		std::strftime(date, sizeof(date), "%Y-%m-%dT%H:%M:%S", &local);

		return "[Trash Info]\nPath=" + PercentEncode(original.string()) + "\nDeletionDate=" + date + "\n";
	}

	bool WriteAll(int file, const std::string& text)
	{
		size_t written = 0;
		while (written < text.size())
		{
			const ssize_t count = ::write(file, text.data() + written, text.size() - written);
			if (count < 0 && errno == EINTR)
			{
				continue;
			}
			if (count <= 0)
			{
				return false;
			}
			written += static_cast<size_t>(count);
		}
		return true;
	}

	// The name item has in the trash: its own, or "Name.2.ext", "Name.3.ext"... if another trashed item has it
	std::string GetTrashName(const std::filesystem::path& item, int number)
	{
		const std::filesystem::path name = item.filename();
		return number == 1 ? name.string() : name.stem().string() + "." + std::to_string(number) + name.extension().string();
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
		std::filesystem::path given = std::filesystem::path(path.GetAbsolutePath()).lexically_normal();
		if (!given.has_filename())
		{
			// A folder's path ending in '/'
			given = given.parent_path();
		}

		// The item itself isn't followed, so a link is trashed as a link
		struct stat itemInfo;
		if (::lstat(given.c_str(), &itemInfo) != 0)
		{
			return false;
		}

		// Its folder without links, so the top of its filesystem is found among the real folders above it
		std::error_code error;
		const std::filesystem::path folder = std::filesystem::canonical(given.parent_path(), error);
		if (error)
		{
			return FailToTrash(given, "its folder can't be resolved (" + error.message() + ")");
		}
		const std::filesystem::path item = folder / given.filename();

		const TrashFolder trash = FindTrashFor(folder, itemInfo.st_dev);
		if (trash.Root.empty())
		{
			return FailToTrash(item, "there's no Trash folder it can be moved to without copying it");
		}

		// The .trashinfo is made first, and only if it isn't there, so two programs trashing the same name at once
		// can't both have it
		const std::string trashInfo = MakeTrashInfo(item, trash.PathsFrom);
		for (int number = 1; number <= 9999; ++number)
		{
			const std::string trashName = GetTrashName(item, number);
			const std::filesystem::path infoFile = trash.Root / "info" / (trashName + ".trashinfo");
			const std::filesystem::path trashed = trash.Root / "files" / trashName;

			const int file = ::open(infoFile.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0600);
			if (file < 0)
			{
				if (errno == EEXIST)
				{
					continue;
				}
				return FailToTrash(item, DescribeError("its .trashinfo can't be made"));
			}

			// Left without a .trashinfo by something else; rename would replace it
			struct stat existing;
			if (::lstat(trashed.c_str(), &existing) == 0)
			{
				::close(file);
				::unlink(infoFile.c_str());
				continue;
			}

			const bool isWritten = WriteAll(file, trashInfo);
			const bool isClosed = ::close(file) == 0;
			if (!isWritten || !isClosed)
			{
				const std::string why = DescribeError("its .trashinfo can't be written");
				::unlink(infoFile.c_str());
				return FailToTrash(item, why);
			}

			if (::rename(item.c_str(), trashed.c_str()) != 0)
			{
				const std::string why = DescribeError("it can't be moved");
				::unlink(infoFile.c_str());
				return FailToTrash(item, why);
			}
			return true;
		}
		return FailToTrash(item, "every name for it in the Trash is taken");
	}
}
