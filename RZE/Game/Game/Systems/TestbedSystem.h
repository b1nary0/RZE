#pragma once

#include <Apollo/ECS/EntitySystem.h>

#include <EngineCore/Resources/ResourceHandler.h>

class TestbedSystem : public Apollo::EntitySystem
{
public:
	TestbedSystem(Apollo::EntityHandler* const entityHandler)
		: Apollo::EntitySystem(entityHandler) {}

public:
	void Initialize() override;
	void Update(const std::vector<Apollo::EntityID>& entities) override;
	void ShutDown() override;

private:
	Apollo::EntityID mCameraEntity;

	ResourceHandle mProjectileResource;
};