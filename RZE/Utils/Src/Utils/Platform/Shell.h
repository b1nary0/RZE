#pragma once

class Filepath;

// What the desktop does with files, behind one interface so callers don't need the OS's headers
namespace Shell
{
	// Opens the OS's file browser (Explorer on Windows) on a folder, or on a file's folder with the file selected.
	// False if it couldn't be opened, e.g. the path doesn't exist.
	bool ShowInFileBrowser(const Filepath& path);

	// Moves a file, or a folder with everything in it, to the Recycle Bin, without asking. False if it couldn't, e.g.
	// the path doesn't exist or a file in it is open elsewhere.
	bool MoveToRecycleBin(const Filepath& path);
}
