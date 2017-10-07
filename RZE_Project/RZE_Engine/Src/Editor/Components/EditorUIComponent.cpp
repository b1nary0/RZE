#include <StdAfx.h>

#include <Editor/Components/EditorUIComponent.h>

EditorUIComponent::EditorUIComponent()
{
	SetName("EditorUIComponent");
}

EditorUIComponent::EditorUIComponent(const std::string& name)
{
	SetName(name);
}

EditorUIComponent::~EditorUIComponent()
{

}

const Vector2D& EditorUIComponent::GetPosition() const
{
	return mPosition;
}

const Vector2D& EditorUIComponent::GetDimensions() const
{
	return mDimensions;
}

void EditorUIComponent::SetPosition(const Vector2D& position)
{
	mPosition = position;
}

void EditorUIComponent::SetDimensions(const Vector2D& dimensions)
{
	mDimensions = dimensions;
}

