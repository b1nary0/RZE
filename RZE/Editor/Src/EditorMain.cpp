#include <EditorApp.h>

int main(int argc, char* argv[])
{
	Editor::EditorApp editor;

	Filepath::SetDirectoryContext(EDirectoryContext::Tools);

	if (argc > 1)
	{
		editor.ParseArguments(argv, argc);
	}
	
	editor.Run();

	return 0;
}