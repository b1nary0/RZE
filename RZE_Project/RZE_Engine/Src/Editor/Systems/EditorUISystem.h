#pragma once

#include <Game/ECS/EntitySystem.h>

#include <Game/ECS/EntityComponentFilter.h>

class EditorUISystem : public IEntitySystem
{
public:
	EditorUISystem(IEntityAdmin* admin);
	~EditorUISystem();

	virtual void Init() override;
	virtual void Update() override;
	virtual void ShutDown() override;

private:
	EntityComponentFilter mRelevantComponents;
};