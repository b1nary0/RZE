#pragma once

#include <Utils/Platform/Filepath.h>
#include <Utils/PrimitiveDefs.h>

#include <string>

namespace Editor
{
	// A folder of rendered thumbnails: one PNG per asset, named after the asset's file. Assets of one kind live in one
	// folder, so their names don't collide.
	class ThumbnailFolder
	{
	public:
		// Relative to the project, like any Filepath, e.g. "ProjectData/SceneThumbnails"
		explicit ThumbnailFolder(const std::string& folder);

	public:
		const Filepath& GetFolder() const { return m_folder; }

		// <folder>/<the asset's file name, without its extension>.png. Invalid for an invalid asset path.
		Filepath GetPathFor(const Filepath& asset) const;

	private:
		Filepath m_folder;
	};

	namespace Thumbnails
	{
		// Thumbnails are square, this many pixels across
		constexpr U32 k_size = 512;

		// Each scene's view through its main camera, in ProjectData/SceneThumbnails
		const ThumbnailFolder& Scenes();
		// Each mesh asset on its own, in ProjectData/MeshThumbnails
		const ThumbnailFolder& Meshes();

		// The image that shows an asset, picked by its extension: a scene's or a mesh's rendered thumbnail, or the file
		// itself for an image. Invalid for other kinds of asset. It may not exist yet, e.g. a scene never captured.
		Filepath FindFor(const Filepath& asset);
	}
}
