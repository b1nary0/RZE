#include <Services/Thumbnails.h>

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
	}
}
