#include <GameApp.h>

int main(int argc, char* argv[])
{
	// Until the game is packaged, it reads its data straight from the source tree
	Filepath::SetDirectoryContext(EDirectoryContext::Source);

	GameApp gGameApp;

	if (argc > 1)
	{
		gGameApp.ParseArguments(argv, argc);
	}

	gGameApp.Run();

	//getchar();
	return 0;
}