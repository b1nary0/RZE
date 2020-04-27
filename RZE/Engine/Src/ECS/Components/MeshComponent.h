#pragma once

#include <Apollo/ECS/EntityComponent.h>

#include <EngineCore/Resources/ResourceHandler.h>

struct MeshComponent final : Apollo::Component<MeshComponent>
{
	MeshComponent()
	{
		ResourcePath = FilePath("Assets/3D/Cube.obj");
	}

	MeshComponent(const FilePath& resourcePath)
		: ResourcePath(resourcePath) {}
	
	virtual void OnEditorInspect(Apollo::EntityID entityID) override;

	virtual void Save(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) override;
	virtual void Load(const rapidjson::Value& data) override;

	FilePath ResourcePath;

	ResourceHandle Resource;
};