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

	FilePath ResourcePath;

	ResourceHandle Resource;
};