#pragma once

#include <Graphics/SceneGraph.h>

#include <Apollo/ECS/EntitySystem.h>

#include <Utils/Math/Matrix4x4.h>

struct CameraComponent;
struct TransformComponent;

class MeshGeometry;
class Model3D;

namespace Diotima
{
	class Renderer;
}

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
	void CreateAndInitializeRenderNode(const Apollo::EntityID entityID, const Model3D& modelData, const Matrix4x4& transform);

private:
	RenderSceneGraph mSceneGraph;

private:
	Diotima::Renderer* mRenderer;

	Apollo::EntityID mCurrentCameraEntity = Apollo::kInvalidEntityID;
};