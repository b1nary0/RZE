#include <SourceAssetBurner/Importers/AssimpSourceImporter.h>

#include <Utils/DebugUtils/Debug.h>

int main(int argc, char* argv[])
{
	if (argc <= 1)
	{
		LOG_CONSOLE("SourceAssetBurner : No arguments provided");
		return -1;
	}

	FilePath::SetDirectoryContext(EDirectoryContext::Tools);
	// #TODO just prototype stuff. assuming the only argument is the source file to import
	// argv[0] is executable path
	FilePath sourcePath(argv[1]);

	AssimpSourceImporter assimpImporter;
	assimpImporter.Import(sourcePath);

	return 0;
}