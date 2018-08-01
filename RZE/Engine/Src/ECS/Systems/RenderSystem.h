#pragma once

#include <Apollo/ECS/EntitySystem.h>

#include <Utils/Functor.h>

struct CameraComponent;
struct TransformComponent;

class RenderSystem final : public Apollo::EntitySystem
{
public:
	RenderSystem(Apollo::EntityHandler* const entityHandler);

public:
	virtual void Initialize() override;
	virtual void Update(const std::vector<Apollo::EntityID>& entities) override;
	virtual void ShutDown() override;

private:
	void RegisterForComponentNotifications();

	void GenerateCameraMatrices(CameraComponent& cameraComponent, const TransformComponent& transformComponent);

private:
	std::unordered_map<Apollo::EntityID, Int32> mRenderItemEntityMap;
	std::unordered_map<Apollo::EntityID, Int32> mLightItemEntityMap;

	// #TODO(Josh) Need a way to determine when the main camera switches. Storing this for now to avoid unnecessary
	// component finds for each possible camera while only working with one for the forseeable future.
	Apollo::EntityID mMainCameraEntity;
};