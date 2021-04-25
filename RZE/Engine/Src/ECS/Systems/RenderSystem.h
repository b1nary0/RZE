#pragma once

#include <Apollo/ECS/EntitySystem.h>

#include <Utils/Functor.h>

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
private:
	struct RenderNode
	{
		U32 RenderObjectIndex;
		Apollo::EntityID EntityID;
		const MeshGeometry* Geometry = nullptr;
		Matrix4x4 Transform = Matrix4x4::IDENTITY;
		std::vector<RenderNode> Children;
	};

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
	Diotima::Renderer* mRenderer;

	std::vector<RenderNode> mRootNodes;

	Apollo::EntityID mCurrentCameraEntity = Apollo::kInvalidEntityID;
};