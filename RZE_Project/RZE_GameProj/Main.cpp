#include "RZE.h"

#include "Game/GameApp.h"

int main(void)
{
	Functor<RZE_Game* const> gameCreateFunc([]()
	{
		return new GameApp();
	});

	GEngine = new RZE_Engine();
	GEngine->Run(gameCreateFunc);

	//getchar();
	return 0;
}