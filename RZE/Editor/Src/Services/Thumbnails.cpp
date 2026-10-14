#include <Services/Thumbnails.h>

#include <Asset/AssetPaths.h>

#include <Utils/StringUtils.h>

#include <algorithm>
#include <filesystem>
#include <functional>
#include <vector>

namespace Editor
{
	ThumbnailFolder::ThumbnailFolder(const std::string& folder)
		: m_folder(folder)
	{
	}

	Filepath ThumbnailFolder::GetPathFor(const Filepath& asset) const
	{
		if (!asset.IsValid())
		{
			return Filepath();
		}

		return Filepath(m_folder.GetRelativePath() + "/" + asset.GetStem() + ".png");
	}

	namespace Thumbnails
	{
		const ThumbnailFolder& Scenes()
		{
			static const ThumbnailFolder s_scenes("ProjectData/SceneThumbnails");
			return s_scenes;
		}

		const ThumbnailFolder& Meshes()
		{
			static const ThumbnailFolder s_meshes("ProjectData/MeshThumbnails");
			return s_meshes;
		}

		Filepath FindFor(const Filepath& asset)
		{
			struct Source
			{
				const char* Extension;
				std::function<Filepath(const Filepath&)> FindImage;
			};

			static const std::vector<Source> s_sources =
			{
				{ AssetPaths::k_sceneExtension, [](const Filepath& file) { return Scenes().GetPathFor(file); } },
				{ AssetPaths::k_meshExtension, [](const Filepath& file) { return Meshes().GetPathFor(file); } },
				// Images show themselves
				{ ".png", [](const Filepath& file) { return file; } },
				{ ".jpg", [](const Filepath& file) { return file; } },
				{ ".jpeg", [](const Filepath& file) { return file; } },
				{ ".tga", [](const Filepath& file) { return file; } },
				{ ".bmp", [](const Filepath& file) { return file; } },
			};

			const std::string extension = asset.GetExtension();
			const auto source = std::find_if(s_sources.begin(), s_sources.end(),
				[&extension](const Source& candidate) { return StringUtils::EqualsIgnoringCase(candidate.Extension, extension); });
			return source != s_sources.end() ? source->FindImage(asset) : Filepath();
		}

		bool IsRendered(const Filepath& asset)
		{
			return IsScene(asset) || StringUtils::EqualsIgnoringCase(asset.GetExtension(), AssetPaths::k_meshExtension);
		}

		bool IsScene(const Filepath& asset)
		{
			return StringUtils::EqualsIgnoringCase(asset.GetExtension(), AssetPaths::k_sceneExtension);
		}

		bool IsUpToDate(const Filepath& asset, const Filepath& image)
		{
			std::error_code imageError;
			std::error_code assetError;
			const std::filesystem::file_time_type imageTime = std::filesystem::last_write_time(image.GetAbsolutePath(), imageError);
			const std::filesystem::file_time_type assetTime = std::filesystem::last_write_time(asset.GetAbsolutePath(), assetError);
			return !imageError && !assetError && imageTime >= assetTime;
		}
	}
}
