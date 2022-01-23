#pragma once

#include <Game/World/GameObject/GameObjectComponent.h>

#include <EngineCore/Resources/ResourceHandler.h>

class MeshComponent : public GameObjectComponent<MeshComponent>
{
	MeshComponent() = default;
	~MeshComponent() = default;

public:
	void SetMeshResource(const ResourceHandle& resource);

	//virtual void OnEditorInspect(Apollo::EntityID entityID) override;

	virtual void Save(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) override;
	virtual void Load(const rapidjson::Value& data) override;

private:
	ResourceHandle m_resource;
};