#pragma once

#include <Game/World/GameObject/GameObjectComponent.h>

#include <Graphics/GraphicsDefines.h>

#include <EngineCore/Resources/ResourceHandler.h>

class RenderComponent : public GameObjectComponent<RenderComponent>
{
public:
	RenderComponent() = default;
	~RenderComponent() = default;

	// GameObjectComponent interface
public:
	void OnAddToScene() override;
	void OnRemoveFromScene() override;

	void Update() override;

	void Serialize(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) override;
	void Deserialize(const rapidjson::Value& data) override;

	static void DescribeProperties(PropertyBuilder<RenderComponent>& properties);

	void OnEditorInspect() override;

public:
	// The mesh asset's path; empty when there's no mesh
	const Filepath& GetMeshPath() const;
	// The one way to give the component a mesh, from a scene file, the editor or code. Loads the mesh asset at path
	// through the ResourceHandler, releases the previous one, and rebuilds the render object from it if the object
	// is in a scene (otherwise OnAddToScene builds it). An empty path unloads the mesh; the path already loaded does
	// nothing.
	void LoadMesh(const Filepath& path);

	// The mesh's size along the object's own axes, including its scale; zero without a mesh
	Vector3D GetMeshSize() const;

	bool IsDrawingMeshBounds() const { return m_drawMeshBounds; }
	void SetDrawMeshBounds(bool draw);
	bool IsDrawingSubMeshBounds() const { return m_drawSubMeshBounds; }
	void SetDrawSubMeshBounds(bool draw) { m_drawSubMeshBounds = draw; }

	// Axis-aligned world-space box around the mesh; false if no mesh has been created yet
	bool GetWorldBounds(Vector3D& outMin, Vector3D& outMax);
	// Null until a mesh has been created
	RenderObjectPtr GetRenderObject() const { return m_renderObject; }

	// See GameObjectComponentRegistry::ResourceDependencyGatherer.
	static void GatherResourceDependencies(const rapidjson::Value& componentData, ResourceDependencyList& outDependencies);

private:
	void CreateRenderObject();
	void DrawBounds();

private:
	RenderObjectPtr m_renderObject;
	ResourceHandle m_resource;

	// Editor debug toggles; not serialized
	bool m_drawMeshBounds = false;
	bool m_drawSubMeshBounds = false;
};