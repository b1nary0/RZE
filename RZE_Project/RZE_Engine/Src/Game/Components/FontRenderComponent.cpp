#include <StdAfx.h>

#include <Game/Components/FontRenderComponent.h>

FontRenderComponent::FontRenderComponent()
{
    SetName("FontRenderComponent");
}

FontRenderComponent::FontRenderComponent(const std::string& name)
{
    SetName(name);
}

FontRenderComponent::~FontRenderComponent()
{
}
