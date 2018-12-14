#pragma once

#include <Apollo/ECS/EntitySystem.h>

struct TransformComponent;

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
	TransformComponent* mWalkerTransform { nullptr };
};
