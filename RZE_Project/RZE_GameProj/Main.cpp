#include "RZE.h"

#include "Game/GameApp.h"

int main(void)
{
	Functor<RZE_Game* const> gameCreateFunc([]()
	{
		return new GameApp();
	});

    RZE_Engine::Get()->Run(gameCreateFunc);

	//getchar();
	return 0;
}