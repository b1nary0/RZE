#pragma once

#include <Game/ECS/EntityComponent.h>

class FontRenderComponent : public IEntityComponent
{
public:
    FontRenderComponent();
    FontRenderComponent(const std::string& name);
    virtual ~FontRenderComponent();
    
private:

};