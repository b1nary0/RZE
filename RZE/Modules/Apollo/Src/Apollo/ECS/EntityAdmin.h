#pragma once

#include <vector>

#include <Utils/DebugUtils/Debug.h>

class IEntity;
class IEntitySystem;

class IEntityAdmin
{
protected:

public:
	typedef std::vector<IEntitySystem*> SystemList;
	typedef std::vector<IEntity*> EntityList;

public:
	virtual void Initialize() = 0;
	virtual void Update() = 0;
	virtual void ShutDown() = 0;

	const SystemList& GetSystems() { return mSystems; }
	const EntityList& GetEntities() { return mEntities; }

	template <class TEntity, typename... Args>
	TEntity* AddEntity(Args... args)
	{
		IEntity* const entity = new TEntity(std::forward<Args>(args)...);
		InternalGetEntities().push_back(entity);
		return static_cast<TEntity*>(InternalGetEntities().back());
	}

	template <class TSystem, typename... Args>
	void AddSystem(Args... args)
	{
		InternalAddSystem<TSystem>(args...);
	}

protected:
	SystemList& InternalGetSystems() { return mSystems; }
	EntityList& InternalGetEntities() { return mEntities; }

	template <class TSystem, typename... Args>
	void InternalAddSystem(Args... args)
	{
		TSystem* system = new TSystem(std::forward<Args>(args)...);
		AssertNotNull(system);
		mSystems.push_back(system);
	}

private:
	SystemList mSystems;
	EntityList mEntities;
};
