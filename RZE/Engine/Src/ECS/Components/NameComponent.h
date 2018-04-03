#pragma once

#include <Apollo/ECS/EntityComponent.h>

class NameComponent : public Apollo::Component<NameComponent>
{
public:
	NameComponent(const std::string& name)
		: Name(name) {}

	std::string Name;
};
