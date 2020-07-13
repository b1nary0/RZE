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
};