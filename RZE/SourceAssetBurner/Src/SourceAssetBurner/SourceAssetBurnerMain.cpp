#include <SourceAssetBurner/Importers/AssimpSourceImporter.h>

#include <Utils/DebugUtils/Debug.h>

#include <algorithm>
#include <cctype>
#include <filesystem>

namespace
{
	// Source models the batch burn picks up
	bool IsSourceModel(const std::filesystem::path& path)
	{
		std::string extension = path.extension().string();
		std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
		return extension == ".obj" || extension == ".fbx";
	}
}

// Exits with 1 if any source failed to import, so a build step running the burn fails with it
int main(int argc, char* argv[])
{
	Filepath::SetDirectoryContext(EDirectoryContext::Source);
	if (argc <= 1)
	{
		// Troll Assets/3D and BURN IT ALL
		// MUAHAHAHAHAHAHAHAHAHAHAHAHHAHA HAH AH AA
		size_t importedCount = 0;
		size_t failedCount = 0;
		Filepath assetsDir3D("Assets/3D/");
		for (const auto& dirEntry : std::filesystem::recursive_directory_iterator(assetsDir3D.GetAbsolutePath()))
		{
			std::string path = dirEntry.path().generic_string();
			if (dirEntry.is_regular_file() && IsSourceModel(dirEntry.path()))
			{
				size_t pos = path.find("Assets/");
				if (pos != std::string::npos)
				{
					path = path.substr(pos, path.size());
					Filepath fp(path);

					AssimpSourceImporter assimpImporter;
					if (assimpImporter.Import(fp))
					{
						++importedCount;
					}
					else
					{
						++failedCount;
					}
				}
			}
		}

		if (failedCount > 0)
		{
			RZE_LOG_ERROR_ARGS("SourceAssetBurner : %zu of %zu sources failed to import", failedCount, importedCount + failedCount);
			return 1;
		}

		RZE_LOG_ARGS("SourceAssetBurner : imported %zu sources", importedCount);
		return 0;
	}
	else
	{
		// #TODO just prototype stuff. assuming the only argument is the source file to import
		// argv[0] is executable path
		Filepath sourcePath(argv[1]);

		if (!sourcePath.IsValid())
		{
			RZE_LOG_ERROR_ARGS("SourceAssetBurner : no source file at [%s]", argv[1]);
			return 1;
		}

		AssimpSourceImporter assimpImporter;
		return assimpImporter.Import(sourcePath) ? 0 : 1;
	}
}