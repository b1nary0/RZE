#include "RZE.h"

#include "Game/GameApp.h"

int main(void)
{
	Functor<RZE_Game* const> appCreateFunc([]()
	{
		return new GameApp();
	});

	GEngine = new RZE_Engine();
	GEngine->Run(appCreateFunc);

	//getchar();
	return 0;
}