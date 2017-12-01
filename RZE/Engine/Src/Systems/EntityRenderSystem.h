#pragma once

#include <Apollo/ECS/EntitySystem.h>
#include <Apollo/ECS/EntityComponentFilter.h>

class EntityRenderSystem : public IEntitySystem
{
public:
	EntityRenderSystem();
	virtual ~EntityRenderSystem();

	virtual void Initialize() override;
	virtual void Update(IEntityAdmin::EntityList& entities) override;
	virtual void ShutDown() override;

private:
	void SetFilterTypes();

	EntityComponentFilter mRelevantComponents;
};
