#pragma once

#include <Game/World/GameObject/GameObjectComponent.h>

#include <Graphics/GraphicsDefines.h>

#include <Utils/Math/BoundingBox.h>

#include <memory>

class RenderScene;
class StaticMeshInstance;

class RenderComponent : public GameObjectComponent<RenderComponent>
{
public:
	RenderComponent() = default;
	~RenderComponent() = default;

	// GameObjectComponent interface
public:
	void Initialize() override;
	void OnAddToScene() override;
	void OnRemoveFromScene() override;

	void Update() override;

	void Serialize(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) override;
	void Deserialize(const rapidjson::Value& data) override;

	static void DescribeProperties(PropertyBuilder<RenderComponent>& properties);

public:
	// The mesh asset's path; empty when there's no mesh, or it was made in code
	const Filepath& GetMeshPath() const;
	// Gives the component the mesh asset at path, loading it synchronously through the ResourceHandler (a cache hit
	// when it was preloaded, as scene loading does), then hands it over with SetMeshInstance. An empty path, or one
	// that fails to load, leaves the component without a mesh; the path already loaded does nothing.
	void LoadMesh(const Filepath& path);
	// The one way the component's mesh changes. Draws meshInstance from now on: the render object, if there is one,
	// keeps its identity and swaps what it draws; one is created if the object is in a scene and had none (otherwise
	// OnAddToScene creates it). Null removes the mesh. The previous instance is released.
	void SetMeshInstance(std::shared_ptr<StaticMeshInstance> meshInstance);

	// The mesh's size along the object's own axes, including its scale; zero without a mesh
	Vector3D GetMeshSize() const;
	// Null without a mesh
	const StaticMeshInstance* GetMesh() const { return m_meshInstance.get(); }

	bool IsDrawingMeshBounds() const { return m_drawMeshBounds; }
	void SetDrawMeshBounds(bool draw);
	bool IsDrawingSubMeshBounds() const { return m_drawSubMeshBounds; }
	void SetDrawSubMeshBounds(bool draw) { m_drawSubMeshBounds = draw; }

	// Axis-aligned world-space box around the mesh; empty if no mesh has been created yet
	BoundingBox GetWorldBounds() const;
	// The mesh's box in the object's own space; empty if no mesh has been created yet
	BoundingBox GetLocalBounds() const override;
	// Null until a mesh has been created. The component keeps the handle; this is for looking at the object.
	const RenderObjectPtr& GetRenderObject() const { return m_renderObject; }

	// See GameObjectComponentRegistry::ResourceDependencyGatherer.
	static void GatherResourceDependencies(const rapidjson::Value& componentData, ResourceDependencyList& outDependencies);

private:
	void CreateRenderObject();
	void DrawBounds();

private:
	// The owner's scene's; set in Initialize, once the component has an owner, and fixed from then on since objects
	// never change scene
	RenderScene* m_renderScene = nullptr;
	RenderObjectPtr m_renderObject;
	// What it draws; shared with m_renderObject. Holds the mesh asset's resource, keeping it loaded.
	std::shared_ptr<StaticMeshInstance> m_meshInstance;

	// Editor debug toggles; not serialized
	bool m_drawMeshBounds = false;
	bool m_drawSubMeshBounds = false;
};