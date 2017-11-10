#pragma once

#include <Game/ECS/EntitySystem.h>
#include <Game/ECS/EntityComponentFilter.h>

class EditorLogSystem : public IEntitySystem
{
public:
	EditorLogSystem(IEntityAdmin* admin);
	virtual ~EditorLogSystem();

	virtual void Init() override;
	virtual void Update() override;
	virtual void ShutDown() override;

private:
	EntityComponentFilter mRelevantComponents;
};