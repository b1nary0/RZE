#pragma once

#include <vector>

#include <EngineApp.h>

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
};