#pragma once

#include <Apollo/ECS/EntityComponent.h>

#include <EngineCore/Resources/ResourceHandler.h>

struct MeshComponent final : Apollo::Component<MeshComponent>
{
	MeshComponent()
	{
		// #TODO
		// This is not a good place for this. Need to either be able to create
		// a MeshComponent without an asset or have a better way to determine the default asset
		ResourcePath = FilePath("ProjectData/Mesh/Cube.meshasset");
	}

	MeshComponent(const FilePath& resourcePath)
		: ResourcePath(resourcePath) {}
	
	virtual void OnEditorInspect(Apollo::EntityID entityID) override;

	virtual void Save(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) override;
	virtual void Load(const rapidjson::Value& data) override;

	FilePath ResourcePath;

	ResourceHandle Resource;
};