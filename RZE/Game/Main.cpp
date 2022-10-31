#include <Game/GameApp.h>

int main(int argc, char* argv[])
{
	GameApp gGameApp;

	if (argc > 1)
	{
		gGameApp.ParseArguments(argv, argc);
	}

	gGameApp.Run();

	//getchar();
	return 0;
}