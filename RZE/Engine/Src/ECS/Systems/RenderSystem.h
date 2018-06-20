#pragma once

#include <Apollo/ECS/EntitySystem.h>

#include <Utils/Functor.h>

struct CameraComponent;
struct TransformComponent;

namespace Apollo
{
	class ComponentHandler;
}

class RenderSystem : public Apollo::EntitySystem
{
public:
	RenderSystem(Apollo::EntityHandler* const entityHandler);

public:
	virtual void Initialize();
	virtual void Update(const std::vector<Apollo::EntityID>& entities);
	virtual void ShutDown();

	void RegisterForComponentNotifications();

	void GenerateCameraMatrices(CameraComponent* const cameraComponent, const TransformComponent* const transformComponent);

private:
	std::unordered_map<Apollo::EntityID, Int32> mRenderItemEntityMap;
	std::unordered_map<Apollo::EntityID, Int32> mLightItemEntityMap;

	Diotima::GLRenderTargetTexture mRenderTargetTexture;
};