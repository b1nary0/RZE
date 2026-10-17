#include <EditorApp.h>

#include <Utils/DebugUtils/Debug.h>
#include <Utils/Platform/CmdLine.h>

int main(int argc, char* argv[])
{
	Editor::EditorApp editor;

	Filepath::SetDirectoryContext(EDirectoryContext::Source);

	if (argc > 1)
	{
		editor.ParseArguments(argv, argc);
	}

	// The editor is a windowed app with no console of its own
	std::string_view consoleValue;
	if (CmdLine::Arguments::Get("-console", consoleValue))
	{
		Debug::OpenConsole();
	}

	editor.Run();

	return 0;
}