#pragma once

#include "Debug/Debug.h"

class IEntity;
class IEntitySystem;

class IEntityAdmin
{
	typedef std::vector<IEntity*> EntityList;
	typedef std::vector<IEntitySystem*> SystemList;

public:
	virtual void Update() = 0;

	const SystemList& GetSystems() { return mSystems; }
	const EntityList& GetEntities() { return mEntities; }

	virtual IEntity* AddEntity(IEntity* const entity) = 0;

protected:
	SystemList& InternalGetSystems() { return mSystems; }
	EntityList& InternalGetEntities() { return mEntities; }

	template <class TSystem, typename... Args>
	void AddSystem(Args... args)
	{
		TSystem* system = new TSystem(std::forward<Args>(args)...);
		AssertNotNull(system);
		mSystems.push_back(system);
	}

private:
	SystemList mSystems;
	EntityList mEntities;
};
