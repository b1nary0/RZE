#pragma once

#include <EngineCore/Resources/ResourceHandler.h>

struct MaterialComponent
{
	MaterialComponent() = default;

	MaterialComponent(const FilePath& resourcePath)
		: ResourcePath(resourcePath)
	{}

	FilePath ResourcePath;
	ResourceHandle Texture;
};