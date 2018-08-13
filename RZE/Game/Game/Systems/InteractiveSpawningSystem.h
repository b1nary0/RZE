#pragma once

#include <Apollo/ECS/EntitySystem.h>

class InteractiveSpawningSystem : public Apollo::EntitySystem
{
public:
	InteractiveSpawningSystem(Apollo::EntityHandler* const entityHandler)
		: Apollo::EntitySystem(entityHandler) {}

public:
	virtual void Initialize() override;
	virtual void Update(const std::vector<Apollo::EntityID>& entities) override;
	virtual void ShutDown() override;

private:
	void HackyInputThing();

};
