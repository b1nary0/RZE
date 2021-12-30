#include <SourceAssetBurner/Importers/AssimpSourceImporter.h>

#include <Utils/DebugUtils/Debug.h>

#include <filesystem>

int main(int argc, char* argv[])
{
	FilePath::SetDirectoryContext(EDirectoryContext::Tools);
	if (argc <= 1)
	{
		// Troll Assets/3D and BURN IT ALL
		// MUAHAHAHAHAHAHAHAHAHAHAHAHHAHA HAH AH AA
		FilePath assetsDir3D("Assets/3D/");
		for (const auto& dirEntry : std::filesystem::recursive_directory_iterator(assetsDir3D.GetAbsolutePath()))
		{
			std::string path = dirEntry.path().generic_string();
			if (path.find(".obj") != std::string::npos)
			{
				size_t pos = path.find("Assets/");
				if (pos != std::string::npos)
				{
					path = path.substr(pos, path.size());
					FilePath fp(path);

					AssimpSourceImporter assimpImporter;
					assimpImporter.Import(fp);
				}
			}
		}

		return 0;
	}
	else
	{
		// #TODO just prototype stuff. assuming the only argument is the source file to import
		// argv[0] is executable path
		FilePath sourcePath(argv[1]);

		if (sourcePath.IsValid())
		{
			AssimpSourceImporter assimpImporter;
			assimpImporter.Import(sourcePath);
		}
		else
		{

		}

		return 0;
	}
}