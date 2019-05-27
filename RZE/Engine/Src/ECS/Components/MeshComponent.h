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
	
	void OnEditorInspect(Apollo::EntityID entityID) override;

	FilePath ResourcePath;

	ResourceHandle Resource;
};