#include "RZE.h"

#include "Game/GameApp.h"

int main(void)
{
	Functor<std::unique_ptr<RZE_Game>> appCreateFunc([]()
	{
		return std::make_unique<GameApp>();
	});

	GEngine = new RZE_Engine();
	GEngine->Run(appCreateFunc);

	return 0;
}