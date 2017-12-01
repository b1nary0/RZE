#pragma once

#include <Apollo/ECS/EntityAdmin.h>

class IEntityComponent;

class IEntitySystem
{
	typedef std::vector<IEntityComponent*> ComponentList;

public:
	IEntitySystem() {};

	virtual void Initialize() = 0;
	virtual void Update(IEntityAdmin::EntityList& entities) = 0;
	virtual void ShutDown() = 0;

	const ComponentList& GetComponents() { return mComponents; }

protected:
	ComponentList& InternalGetComponents() { return mComponents; }

	ComponentList mComponents;
};
