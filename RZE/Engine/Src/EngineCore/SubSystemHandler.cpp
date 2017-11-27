#include <StdAfx.h>

#include <EngineCore/SubSystemHandler.h>

SubSystemHandler::SubSystemHandler()
{
}

SubSystemHandler::~SubSystemHandler()
{
}

void SubSystemHandler::InitializeSubSystems()
{
	LOG_CONSOLE("Initializing SubSystems.");
	for (size_t idx = 0; idx < mSubSystems.size(); idx++)
	{
		mSubSystems[idx]->Initialize();
	}
}

void SubSystemHandler::UpdateSubSystems()
{
	for (size_t idx = 0; idx < mSubSystems.size(); idx++)
	{
		mSubSystems[idx]->Update();
	}
}

void SubSystemHandler::ShutDownSubSystems()
{
	LOG_CONSOLE("Shutting Down SubSystems.");
	std::reverse(mSubSystems.begin(), mSubSystems.end());
	for (size_t idx = 0; idx < mSubSystems.size(); idx++)
	{
		mSubSystems[idx]->ShutDown();
	}
}
