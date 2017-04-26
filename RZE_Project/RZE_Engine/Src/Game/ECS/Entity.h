#pragma once

class IEntityComponent;

class IEntity
{
	typedef std::vector<IEntityComponent*> ComponentList;

public:
	virtual void Update() = 0;

	const ComponentList& GetComponents() { return mComponents; }

protected:
	ComponentList& InternalGetComponents() { return mComponents; }

private:
	ComponentList mComponents;
	
};
