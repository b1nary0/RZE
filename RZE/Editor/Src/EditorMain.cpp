#include <EditorApp.h>

int main(int argc, char* argv[])
{
	Editor::EditorApp editor;

	Filepath::SetDirectoryContext(EDirectoryContext::Source);

	if (argc > 1)
	{
		editor.ParseArguments(argv, argc);
	}

	editor.Run();

	return 0;
}