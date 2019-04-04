#pragma once

#include <Apollo./ECS/EntitySystem.h>

class ProjectileSystem : public Apollo::EntitySystem
{
public:
	ProjectileSystem(Apollo::EntityHandler* const entityHandler)
		: Apollo::EntitySystem(entityHandler) {}

public:
	void Initialize() override;
	void Update(const std::vector<Apollo::EntityID>& entities) override;
	void ShutDown() override;

private:
	Apollo::EntityID mCameraEntity;
};