#pragma once

#include <Apollo/ECS/EntitySystem.h>

#include <EngineCore/Resources/ResourceHandler.h>

class ProjectileSystem : public Apollo::EntitySystem
{
public:
	ProjectileSystem(Apollo::EntityHandler* const entityHandler)
		: Apollo::EntitySystem(entityHandler) {}

public:
	void Initialize() override;
	void Update(const std::vector<Apollo::EntityID>& entities) override;
	void ShutDown() override;

	// Component lifecycle functions
private:
	void RegisterForComponentNotifications();

	void OnCameraComponentAdded(Apollo::EntityID entityID);

private:
	Apollo::EntityID mCameraEntity;

	ResourceHandle mProjectileResource;
};