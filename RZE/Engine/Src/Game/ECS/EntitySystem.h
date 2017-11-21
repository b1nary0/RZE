#pragma once

class IEntityAdmin;
class IEntityComponent;

class IEntitySystem
{
	typedef std::vector<IEntityComponent*> ComponentList;

public:
	IEntitySystem(IEntityAdmin* admin)
		: mAdmin(admin) {}

	virtual void Init() = 0;
	virtual void Update() = 0;
	virtual void Render() {}
	virtual void ShutDown() = 0;

	const ComponentList& GetComponents() { return mComponents; }

protected:
	ComponentList& InternalGetComponents() { return mComponents; }

	ComponentList mComponents;
	IEntityAdmin* mAdmin;
};
