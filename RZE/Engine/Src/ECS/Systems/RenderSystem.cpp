#include <StdAfx.h>

#include <ECS/Systems/RenderSystem.h>

#include <ECS/Components/TransformComponent.h>

#include <Apollo/ECS/EntityComponentFilter.h>

#include <Utils/Platform/Timers/HiResTimer.h>

Apollo::EntityComponentFilter componentFilter;

RenderSystem::RenderSystem()
{
}

void RenderSystem::Initialize()
{
	componentFilter.AddFilterType<TransformComponent>();
}

void RenderSystem::Update(std::vector<Apollo::Entity>& entities)
{
	HiResTimer entityFilterTimer;

	entityFilterTimer.Start();
		Apollo::ComponentHandler::EntityList relevantEntities;
		componentFilter.FilterEachOf(entities, relevantEntities);
	entityFilterTimer.Stop();

	static bool bPrintTest = true;
	if (bPrintTest)
	{
		LOG_CONSOLE_ARGS("Entity Filter took %f ms with %i entities.", entityFilterTimer.GetElapsed<float>() * 1000.0f, static_cast<int>(entities.size()));
		bPrintTest = false;
	}
}

void RenderSystem::ShutDown()
{
}
