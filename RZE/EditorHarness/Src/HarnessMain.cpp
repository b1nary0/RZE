#include <HarnessApp.h>

#include <Utils/Platform/Filepath.h>

// As EditorMain.cpp, with the harness's app
int main(int argc, char* argv[])
{
	EditorHarness::HarnessApp editor;

	Filepath::SetDirectoryContext(EDirectoryContext::Source);

	if (argc > 1)
	{
		editor.ParseArguments(argv, argc);
	}

	editor.Run();

	return 0;
}
