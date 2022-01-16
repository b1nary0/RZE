#pragma once

#include <string>

class NameComponent
{
public:
	NameComponent() {}
	NameComponent(const std::string& name)
		: Name(name) {}

	//void OnEditorInspect(Apollo::EntityID entityID) override;

	std::string Name;
};
