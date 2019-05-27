#pragma once

#include <Apollo/ECS/EntityComponent.h>

#include <EngineCore/Resources/ResourceHandler.h>

struct MaterialComponent final : public Apollo::Component<MaterialComponent>
{
	MaterialComponent() = default;

	MaterialComponent(const FilePath& resourcePath)
		: ResourcePath(resourcePath)
	{}

	FilePath ResourcePath;
	ResourceHandle Texture;
};