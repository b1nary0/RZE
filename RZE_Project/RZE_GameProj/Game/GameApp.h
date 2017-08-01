#pragma once

#include <vector>

#include <Application.h>

class GameEntity;
class MeshData;
class ResourceHandle;

class GameApp : public RZE_Game
{
public:

    GameApp();
    virtual ~GameApp();

    virtual void RegisterEvents(EventHandler& eventHandler) override;

    virtual void Start() override;
    virtual void Update() override;

private:
    void CreateLight(const ResourceHandle& resourceHandle);
    void CreateGround(const ResourceHandle& resourceHandle);
    void CreateLampObjects(const ResourceHandle& resourceHandle);
    void CreateTextureQuad(const ResourceHandle& resourceHandle);

    GameEntity* mLightEntity;
    std::vector<GameEntity*> mEntities;
};