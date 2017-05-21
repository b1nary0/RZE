#include "StdAfx.h"

#include "Game/Systems/MovementSystem.h"

MovementSystem::MovementSystem(IEntityAdmin* const admin)
	: IEntitySystem(admin)
{
}

MovementSystem::~MovementSystem()
{
}

void MovementSystem::Update()
{
	for (auto& entity : mAdmin->GetEntities())
	{

	}
}