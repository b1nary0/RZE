#pragma once

#include "Game/ECS/EntitySystem.h"

#include <RenderCore/Graphics/Font/FontHandler.h>

class GFXShaderGroup;

class RenderSystem : public IEntitySystem
{
public:
    RenderSystem(IEntityAdmin* const admin);
    virtual ~RenderSystem();

    virtual void Init() override;
    virtual void Update() override;
    virtual void ShutDown() override;

private:
    GFXShaderGroup* mShaderGroup;
};
