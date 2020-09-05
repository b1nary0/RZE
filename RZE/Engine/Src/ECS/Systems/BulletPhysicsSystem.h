#pragma once

#include <Apollo/ECS/EntitySystem.h>

class BulletPhysicsSystem final : public Apollo::EntitySystem
{
public:
	BulletPhysicsSystem(Apollo::EntityHandler* const entityHandler);
	virtual ~BulletPhysicsSystem();

	void Initialize() override;
	void Update(const std::vector<Apollo::EntityID>& entities) override;
	void ShutDown() override;
};