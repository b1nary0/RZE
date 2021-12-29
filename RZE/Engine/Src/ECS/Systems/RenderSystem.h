#pragma once

#include <Apollo/ECS/EntitySystem.h>

#include <Utils/Math/Matrix4x4.h>

struct CameraComponent;
struct MeshComponent;
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
		Int32 NodeIndex; // Used to index mRootNodes for the moment

		U32 RenderObjectIndex;
		Apollo::EntityID EntityID;
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

	void CreateRenderObject(const MeshComponent& meshComponent, const TransformComponent& transformComponent);
	void DestroyRenderNode(Apollo::EntityID entityID);

	// Component lifecycle callbacks
private:
	void OnMeshComponentAdded(Apollo::EntityID entityID);
	void OnMeshComponentRemoved(Apollo::EntityID entityID);
	void OnMeshComponentModified(Apollo::EntityID entityID);

	void OnCameraComponentAdded(Apollo::EntityID entityID);
	void OnCameraComponentRemoved(Apollo::EntityID entityID);
	void OnCameraComponentModified(Apollo::EntityID entityID);

private:
	Diotima::Renderer* mRenderer;

	std::vector<RenderNode> mRootNodes;

	Apollo::EntityID mCurrentCameraEntity = Apollo::kInvalidEntityID;
};