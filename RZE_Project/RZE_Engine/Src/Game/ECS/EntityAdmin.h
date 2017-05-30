#pragma once

#include "Debug/Debug.h"

class IEntity;
class IEntitySystem;

class RZE_Renderer;

class IEntityAdmin
{
	typedef std::vector<IEntity*> EntityList;
	typedef std::vector<IEntitySystem*> SystemList;

public:
	virtual void Init() = 0;
	virtual void Update() = 0;

	virtual RZE_Renderer* const GetRenderer() const = 0;

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
