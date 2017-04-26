#pragma once

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

protected:
	SystemList& InternalGetSystems() { return mSystems; }
	EntityList& InternalGetEntities() { return mEntities; }

private:
	SystemList mSystems;
	EntityList mEntities;
};
