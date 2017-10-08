#pragma once

#include <Game/ECS/Entity.h>

class EditorEntity : public IEntity
{
public:
	EditorEntity();
	EditorEntity(const std::string& entityName);
	virtual ~EditorEntity();

private:
};