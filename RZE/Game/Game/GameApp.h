#pragma once

#include <EngineApp.h>

#include <Apollo/EntityHandler.h>

#include <vector>

class ResourceHandle;

namespace Diotima
{
	class GFXShaderGroup;
}

class GameApp : public RZE_Application
{
public:

	GameApp();
	virtual ~GameApp();

	virtual void Initialize() override;
	virtual void Start() override;
	virtual void Update() override;

private:
	std::vector<Apollo::EntityID> mNanosuits;
};