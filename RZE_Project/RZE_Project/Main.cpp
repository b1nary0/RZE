#include "StdAfx.h"

#include "Src/Application.h"
#include "Src/EngineCore/Engine.h"

int main(void)
{
	Functor <std::unique_ptr<RZE_Application>> createApplicationFunctor([]() ->std::unique_ptr<RZE_Application>
	{
		return std::make_unique<RZE_Application>();
	});

	RZE_EngineCore rzeInstance;
	rzeInstance.Run(createApplicationFunctor);

	return 0;
}