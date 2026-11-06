#include <Services/AssetFiles.h>

#include <Services/Thumbnails.h>

#include <Asset/AssetPaths.h>

#include <Utils/Platform/File.h>
#include <Utils/Platform/Folders.h>
#include <Utils/Platform/Shell.h>
#include <Utils/StringUtils.h>

#include <filesystem>
#include <fstream>

namespace
{
	// In each of the trash's numbered folders: the deleted file or folder, its thumbnails, and a text file holding the
	// path it was at
	constexpr char k_itemFolder[] = "Item";
	constexpr char k_thumbnailsFolder[] = "Thumbnails";
	constexpr char k_originalFile[] = "Original";

	// The rendered thumbnails of path, or of the scenes and meshes anywhere in it if it's a folder
	std::vector<Filepath> FindThumbnailsOf(const Filepath& path)
	{
		std::error_code error;
		if (!std::filesystem::is_directory(path.GetAbsolutePath(), error))
		{
			return Editor::Thumbnails::IsRendered(path) ? std::vector<Filepath>{ Editor::Thumbnails::FindFor(path) } : std::vector<Filepath>();
		}

		std::vector<Filepath> thumbnails;
		for (const char* extension : { AssetPaths::k_sceneExtension, AssetPaths::k_meshExtension })
		{
			for (const Filepath& asset : Folders::FindFiles(path, extension))
			{
				thumbnails.push_back(Editor::Thumbnails::FindFor(asset));
			}
		}
		return thumbnails;
	}

	// A new, empty numbered folder in the trash, or invalid if one couldn't be made
	Filepath MakeTrashEntry()
	{
		const Filepath& trash = Editor::AssetFiles::GetTrashFolder();
		std::error_code error;
		std::filesystem::create_directories(trash.GetAbsolutePath(), error);

		for (int number = 1; ; ++number)
		{
			const Filepath entry(trash.GetRelativePath() + "/" + std::to_string(number));
			// False, without an error, if it's already there
			if (std::filesystem::create_directory(entry.GetAbsolutePath(), error))
			{
				return entry;
			}
			if (error)
			{
				return Filepath();
			}
		}
	}

	// The numbered folder in the trash that item is in
	Filepath GetTrashEntry(const Editor::TrashedItem& item)
	{
		return item.InTrash.GetParentFolder().GetParentFolder();
	}

	void RemoveAll(const Filepath& path)
	{
		std::error_code ignored;
		std::filesystem::remove_all(path.GetAbsolutePath(), ignored);
	}

	Filepath GetOriginalFile(const Filepath& entry)
	{
		return Filepath(entry.GetRelativePath() + "/" + k_originalFile);
	}

	// Records in entry where its item was, for EmptyTrash. Without it, the item is sent to the Recycle Bin from the trash.
	void WriteOriginal(const Filepath& entry, const Filepath& original)
	{
		std::ofstream(GetOriginalFile(entry).GetAbsolutePath()) << original.GetRelativePath();
	}

	// Where entry's item was, or invalid if it wasn't recorded
	Filepath ReadOriginal(const Filepath& entry)
	{
		std::ifstream stream(GetOriginalFile(entry).GetAbsolutePath());
		std::string path;
		// An empty path would make a valid Filepath, of the executable's folder
		if (!std::getline(stream, path) || path.empty())
		{
			return Filepath();
		}
		return Filepath(path);
	}

	// Sends inTrash to the Recycle Bin from original, where it was, so the Recycle Bin records that folder. It's sent from
	// the trash instead if original is invalid or another item's, something else has its name now, or the folder it was
	// in has gone. False if it couldn't, and it's put back in the trash then.
	bool SendOn(const Filepath& inTrash, const Filepath& original, Editor::AssetFiles::SendToRecycleBin sendToRecycleBin)
	{
		// File::Move refuses a name that's taken, and a folder that isn't there
		const bool isBack = original.IsValid() && original.GetFileName() == inTrash.GetFileName() && File::Move(inTrash, original);
		if (!isBack)
		{
			return sendToRecycleBin(inTrash);
		}

		if (sendToRecycleBin(original))
		{
			return true;
		}

		// For EmptyTrash to try again; if this fails too, it stays where it was, as an undo would leave it
		File::Move(original, inTrash);
		return false;
	}
}

namespace Editor
{
	namespace AssetFiles
	{
		bool Rename(const Filepath& from, const Filepath& to)
		{
			if (!File::Move(from, to))
			{
				return false;
			}

			// Thumbnails are named after their asset, so a folder's are unchanged
			if (Thumbnails::IsRendered(from))
			{
				const Filepath oldImage = Thumbnails::FindFor(from);
				const Filepath newImage = Thumbnails::FindFor(to);
				// Another asset of the same name elsewhere can have newImage; this one's is then rendered again when shown
				if (oldImage.Exists() && !File::Move(oldImage, newImage))
				{
					RemoveAll(oldImage);
				}
			}
			return true;
		}

		bool CreateFolder(const Filepath& folder)
		{
			// False, without an error, if it's already there
			std::error_code error;
			return std::filesystem::create_directory(folder.GetAbsolutePath(), error);
		}

		std::string GetWhyInUse(const Filepath& path, const Filepath& openScene)
		{
			if (!openScene.IsValid())
			{
				return std::string();
			}

			if (StringUtils::EqualsIgnoringCase(openScene.GetRelativePath(), path.GetRelativePath()))
			{
				return "It's the open scene";
			}
			return openScene.IsInside(path) ? "The open scene is in it" : std::string();
		}

		TrashedItem MoveToTrash(const Filepath& path)
		{
			TrashedItem item;
			item.Original = path;
			if (!path.Exists())
			{
				return item;
			}

			// Found first, since a folder's contents can't be listed once it's moved
			const std::vector<Filepath> thumbnails = FindThumbnailsOf(path);

			const Filepath entry = MakeTrashEntry();
			if (!entry.IsValid())
			{
				return item;
			}

			// Under its own name, so it keeps it in the Recycle Bin
			const Filepath itemFolder(entry.GetRelativePath() + "/" + k_itemFolder);
			const Filepath inTrash(itemFolder.GetRelativePath() + "/" + path.GetFileName());
			std::error_code error;
			std::filesystem::create_directory(itemFolder.GetAbsolutePath(), error);
			if (error || !File::Move(path, inTrash))
			{
				RemoveAll(entry);
				return item;
			}
			item.InTrash = inTrash;
			WriteOriginal(entry, path);

			const Filepath thumbnailsFolder(entry.GetRelativePath() + "/" + k_thumbnailsFolder);
			for (const Filepath& thumbnail : thumbnails)
			{
				if (!thumbnail.Exists())
				{
					continue;
				}

				// Numbered, since a scene and a mesh can have thumbnails of the same name
				const Filepath thumbnailInTrash(thumbnailsFolder.GetRelativePath() + "/" + std::to_string(item.Thumbnails.size()) + " " + thumbnail.GetFileName());
				std::filesystem::create_directory(thumbnailsFolder.GetAbsolutePath(), error);
				if (File::Move(thumbnail, thumbnailInTrash))
				{
					item.Thumbnails.push_back({ thumbnail, thumbnailInTrash });
				}
				else
				{
					// Rendered again if the asset comes back
					RemoveAll(thumbnail);
				}
			}
			return item;
		}

		bool RestoreFromTrash(const TrashedItem& item)
		{
			if (!item.IsInTrash() || item.Original.Exists() || !File::Move(item.InTrash, item.Original))
			{
				return false;
			}

			for (const TrashedItem::Thumbnail& thumbnail : item.Thumbnails)
			{
				// Another asset of the same name may have rendered one since; this one's is then rendered again when shown
				if (!File::Move(thumbnail.InTrash, thumbnail.Original))
				{
					RemoveAll(thumbnail.InTrash);
				}
			}

			RemoveAll(GetTrashEntry(item));
			return true;
		}

		bool Recycle(const TrashedItem& item)
		{
			return Recycle(item, &Shell::MoveToRecycleBin);
		}

		bool Recycle(const TrashedItem& item, SendToRecycleBin sendToRecycleBin)
		{
			if (!item.IsInTrash() || !SendOn(item.InTrash, item.Original, sendToRecycleBin))
			{
				return false;
			}

			// Its thumbnails go with the folder
			RemoveAll(GetTrashEntry(item));
			return true;
		}

		void EmptyTrash()
		{
			EmptyTrash(&Shell::MoveToRecycleBin);
		}

		void EmptyTrash(SendToRecycleBin sendToRecycleBin)
		{
			const Filepath& trash = GetTrashFolder();
			for (const Filepath& entry : Folders::ListSubfolders(trash))
			{
				const Filepath itemFolder(entry.GetRelativePath() + "/" + k_itemFolder);
				const Filepath original = ReadOriginal(entry);
				bool isRecycled = true;

				std::error_code error;
				for (const std::filesystem::directory_entry& item : std::filesystem::directory_iterator(itemFolder.GetAbsolutePath(), error))
				{
					const Filepath itemPath(itemFolder.GetRelativePath() + "/" + item.path().filename().string());
					isRecycled = SendOn(itemPath, original, sendToRecycleBin) && isRecycled;
				}

				// Left for next time if something in it couldn't go, e.g. it's open in another program
				if (isRecycled)
				{
					RemoveAll(entry);
				}
			}

			// Only if it's empty
			std::error_code ignored;
			std::filesystem::remove(trash.GetAbsolutePath(), ignored);
		}

		const Filepath& GetTrashFolder()
		{
			// Built on first use rather than at static initialization, so it resolves against the project folder
			static const Filepath trash("ProjectData/.Trash");
			return trash;
		}
	}
}
