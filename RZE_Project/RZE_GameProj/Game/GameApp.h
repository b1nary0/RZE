#pragma once

#include <vector>

#include <Application.h>

class GameEntity;
class MeshData;

class GameApp : public RZE_Game
{
public:

    GameApp();
    virtual ~GameApp();

    virtual void RegisterEvents(EventHandler& eventHandler) override;

    virtual void Start() override;
    virtual void Update() override;

private:
    void CreateLight(MeshData* const meshData);
    void CreateGround(MeshData* const meshData);
    void CreateMiniCooper(MeshData* const meshData);
    void CreateLampObjects(MeshData* const meshData);

    GameEntity* mLightEntity;
    std::vector<GameEntity*> mEntities;
};