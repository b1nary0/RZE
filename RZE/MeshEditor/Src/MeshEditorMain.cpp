#include <MeshEditor.h>

int main(int argc, char* argv[])
{
	MeshEditor::MeshEditorApp meshEditor;

	Filepath::SetDirectoryContext(EDirectoryContext::Tools);

	if (argc > 1)
	{
		meshEditor.ParseArguments(argv, argc);
	}

	meshEditor.Run();

	return 0;
}