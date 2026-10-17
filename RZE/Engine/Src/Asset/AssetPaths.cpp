#include <StdAfx.h>
#include <Asset/AssetPaths.h>

#include <Utils/StringUtils.h>

namespace AssetPaths
{
	const Filepath& GetSceneFolder()
	{
		static const Filepath s_sceneFolder("Assets/Scenes");
		return s_sceneFolder;
	}

	bool IsScene(const Filepath& file)
	{
		return StringUtils::EqualsIgnoringCase(file.GetExtension(), k_sceneExtension);
	}

	Filepath GetScene(const std::string& fileName)
	{
		return Filepath(GetSceneFolder().GetRelativePath() + "/" + fileName);
	}

	std::string GetSceneName(const Filepath& scene)
	{
		const Filepath& sceneFolder = GetSceneFolder();
		if (!scene.IsInside(sceneFolder) || scene.GetRelativePath().size() == sceneFolder.GetRelativePath().size())
		{
			return std::string();
		}

		// Past the folder and the separator after it
		return scene.GetRelativePath().substr(sceneFolder.GetRelativePath().size() + 1);
	}

	const Filepath& GetMeshFolder()
	{
		static const Filepath s_meshFolder("ProjectData/Mesh");
		return s_meshFolder;
	}

	bool IsMesh(const Filepath& file)
	{
		return StringUtils::EqualsIgnoringCase(file.GetExtension(), k_meshExtension);
	}

	const Filepath& GetMaterialFolder()
	{
		static const Filepath s_materialFolder("ProjectData/Material");
		return s_materialFolder;
	}
}
