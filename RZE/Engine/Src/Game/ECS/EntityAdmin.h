#pragma once

#include "DebugUtils/Debug.h"

class IEntity;
class IEntitySystem;

class RZE_Renderer;

class IEntityAdmin
{
protected:
	typedef std::vector<IEntity*> EntityList;
	typedef std::vector<IEntitySystem*> SystemList;

public:
	virtual void Init() = 0;
	virtual void Update() = 0;
	virtual void ShutDown() = 0;

	virtual RZE_Renderer* const GetRenderer() const = 0;

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
		InternalAddSystem<TSystem>(args);
	}

protected:
	SystemList& InternalGetSystems() { return mSystems; }
	EntityList& InternalGetEntities() { return mEntities; }

	virtual void InternalSetRenderer(RZE_Renderer* const renderer) = 0;

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
