#include <StdAfx.h>
#include <Asset/AssetPaths.h>

namespace AssetPaths
{
	const Filepath& GetSceneFolder()
	{
		static const Filepath s_sceneFolder("Assets/Scenes");
		return s_sceneFolder;
	}

	Filepath GetScene(const std::string& fileName)
	{
		return Filepath(GetSceneFolder().GetRelativePath() + "/" + fileName);
	}

	const Filepath& GetMeshFolder()
	{
		static const Filepath s_meshFolder("ProjectData/Mesh");
		return s_meshFolder;
	}

	const Filepath& GetMaterialFolder()
	{
		static const Filepath s_materialFolder("ProjectData/Material");
		return s_materialFolder;
	}
}
