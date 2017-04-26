#pragma once

class IEntityComponent;

class IEntitySystem
{
	typedef std::vector<IEntityComponent*> ComponentList;

public:
	virtual void Update() = 0;

	const ComponentList& GetComponents() { return mComponents; }

protected:
	ComponentList& InternalGetComponents() { return mComponents; }

	ComponentList mComponents;
};