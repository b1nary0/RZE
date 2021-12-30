#include <SourceAssetBurner/Importers/AssimpSourceImporter.h>

#include <Utils/DebugUtils/Debug.h>

#include <filesystem>

int main(int argc, char* argv[])
{
	LOG_CONSOLE("SOURCE ASSET BURNER");
	FilePath::SetDirectoryContext(EDirectoryContext::Tools);
	if (argc <= 1)
	{
		LOG_CONSOLE("ARGS <= 1");
		// Troll Assets/3D and BURN IT ALL
		// MUAHAHAHAHAHAHAHAHAHAHAHAHHAHA HAH AH AA
		FilePath assetsDir3D("Assets/3D/");
		LOG_CONSOLE_ARGS("ASSET PATH ABS: %s", assetsDir3D.GetAbsolutePath().c_str());
		for (const auto& dirEntry : std::filesystem::recursive_directory_iterator(assetsDir3D.GetAbsolutePath()))
		{
			LOG_CONSOLE_ARGS("DIR ENTRY: %s", dirEntry.path().generic_string().c_str());

			std::string path = dirEntry.path().generic_string();
			if (path.find(".obj") != std::string::npos)
			{
				LOG_CONSOLE("FOUND OBJ");
				size_t pos = path.find("Assets/");
				if (pos != std::string::npos)
				{
					path = path.substr(pos, path.size());
					FilePath fp(path);

					LOG_CONSOLE_ARGS("FOUND PATH: %s", fp.GetAbsolutePath().c_str());

					AssimpSourceImporter assimpImporter;
					assimpImporter.Import(fp);
				}
			}
		}
		LOG_CONSOLE("AFTER LOOP");

		return 0;
	}
	else
	{
		LOG_CONSOLE("ARGS > 1");
		// #TODO just prototype stuff. assuming the only argument is the source file to import
		// argv[0] is executable path
		FilePath sourcePath(argv[1]);

		if (sourcePath.IsValid())
		{
			LOG_CONSOLE("SOURCE PATH VALID");
			AssimpSourceImporter assimpImporter;
			assimpImporter.Import(sourcePath);
		}

		return 0;
	}
}