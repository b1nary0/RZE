#pragma once

#include <Apollo/ECS/EntityComponent.h>

#include <EngineCore/Resources/ResourceHandler.h>

#include <Diotima/Shaders/ShaderGroup.h>

struct MaterialComponent : public Apollo::Component<MaterialComponent>
{
	MaterialComponent() = default;
	
	MaterialComponent(const FilePath& resourcePath)
		: ResourcePath(resourcePath)
	{}

	FilePath ResourcePath;
	ResourceHandle Texture;
	Diotima::GFXShaderGroup* ShaderGroup;
};