#include <Services/AssetFiles.h>

#include <Services/FolderListing.h>
#include <Services/Thumbnails.h>

#include <Asset/AssetPaths.h>

#include <Utils/Platform/File.h>
#include <Utils/Platform/Shell.h>

#include <filesystem>
#include <vector>

namespace
{
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
			for (const Filepath& asset : Editor::FindFiles(path, extension))
			{
				thumbnails.push_back(Editor::Thumbnails::FindFor(asset));
			}
		}
		return thumbnails;
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
					std::error_code ignored;
					std::filesystem::remove(oldImage.GetAbsolutePath(), ignored);
				}
			}
			return true;
		}

		bool Delete(const Filepath& path)
		{
			// Found first, since a folder's contents can't be listed once it's gone
			const std::vector<Filepath> thumbnails = FindThumbnailsOf(path);
			if (!Shell::MoveToRecycleBin(path))
			{
				return false;
			}

			for (const Filepath& thumbnail : thumbnails)
			{
				std::error_code ignored;
				std::filesystem::remove(thumbnail.GetAbsolutePath(), ignored);
			}
			return true;
		}

		Filepath CreateFolder(const Filepath& parent, const std::string& name)
		{
			const Filepath folder = MakeUniqueFileName(parent, name);

			std::error_code error;
			return std::filesystem::create_directory(folder.GetAbsolutePath(), error) ? folder : Filepath();
		}
	}
}
