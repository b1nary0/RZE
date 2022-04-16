#pragma once

#include <Game/World/GameObject/GameObjectComponent.h>

#include <Graphics/GraphicsDefines.h>

#include <EngineCore/Resources/ResourceHandler.h>

class MeshComponent : public GameObjectComponent<MeshComponent>
{
public:
	MeshComponent() = default;
	~MeshComponent() = default;

	// GameObjectComponent interface
public:
	void OnAddToScene() override;
	void OnRemoveFromScene() override;

	void Update() override;

	void Save(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) override;
	void Load(const rapidjson::Value& data) override;

	void OnEditorInspect() override;

public:
	void SetMeshResource(const ResourceHandle& resource);
	
private:
	void CreateRenderObject();

private:
	RenderObjectPtr m_renderObject;
	ResourceHandle m_resource;
};