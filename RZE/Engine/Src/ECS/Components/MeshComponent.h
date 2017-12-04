#pragma once

#include <Apollo/ECS/EntityComponent.h>

#include <EngineCore/Resources/ResourceHandler.h>

struct MeshComponent : Apollo::Component<MeshComponent>
{
	MeshComponent(const std::string& resourcePath)
		: ResourcePath(resourcePath)
	{}

	const std::string ResourcePath;

	ResourceHandle Resource;
};