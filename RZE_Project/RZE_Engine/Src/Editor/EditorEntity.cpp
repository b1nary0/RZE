#include <StdAfx.h>

#include <Editor/EditorEntity.h>

EditorEntity::EditorEntity()
{
}

EditorEntity::EditorEntity(const std::string& entityName)
	: IEntity(entityName)
{
}

EditorEntity::~EditorEntity()
{
}
