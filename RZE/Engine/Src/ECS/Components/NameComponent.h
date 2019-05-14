#pragma once

#include <Apollo/ECS/EntityComponent.h>

class NameComponent final : public Apollo::Component<NameComponent>
{
public:
	NameComponent() {}
	NameComponent(const std::string& name)
		: Name(name) {}

	std::string Name;
};
