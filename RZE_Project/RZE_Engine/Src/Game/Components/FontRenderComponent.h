#pragma once

#include <Game/ECS/EntityComponent.h>

class FontRenderComponent : public IEntityComponent
{
public:
    FontRenderComponent();
    FontRenderComponent(const std::string& name);
    virtual ~FontRenderComponent();
    
    void SetFont(const std::string& fontName);
    void SetText(const std::string& text);

private:
    std::string mText;

};