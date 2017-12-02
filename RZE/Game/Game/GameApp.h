#pragma once

#include <vector>

#include <EngineApp.h>

class GameEntity;
class MeshData;
class ResourceHandle;

namespace Diotima
{
	class GFXShaderGroup;
}

class GameApp : public RZE_Game
{
public:

	GameApp();
	virtual ~GameApp();

	virtual void Start() override;
	virtual void Update() override;

private:
	void CreateLight(const ResourceHandle& resourceHandle);
	void CreateGround(const ResourceHandle& resourceHandle);
	void CreateLampObjects(const ResourceHandle& resourceHandle);
	void CreateTextureQuad(const ResourceHandle& meshHandle, const ResourceHandle& textureHandle);

	void CreateDefaultShader();
	void CreateTextureShader();

	GameEntity* mLightEntity;

	Diotima::GFXShaderGroup* mDefaultShader;
	Diotima::GFXShaderGroup* mTextureShader;

	std::vector<GameEntity*> mEntities;
};