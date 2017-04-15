#include "EngineCore/Engine.h"

#include "Game/GameApp.h"

int main(void)
{
	Functor<std::unique_ptr<RZE_Application>> appCreateFunc([]()
	{
		return std::make_unique<GameApp>();
	});

	RZE_EngineCore engine;
	engine.Run(appCreateFunc);

	return 0;
}