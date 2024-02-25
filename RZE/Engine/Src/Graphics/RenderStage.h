#pragma once

#include <Graphics/RenderEngine.h>

#include <Utils/PrimitiveDefs.h>

#include <memory>
#include <vector>

class RenderObject;
class LightObject;
struct RenderCamera;

class IRenderStage
{
public:
	IRenderStage() {};
	virtual ~IRenderStage() {};

public:
	virtual void Initialize() = 0;
	virtual void Update(const RenderCamera& camera, const RenderEngine::SceneData& renderData) = 0;
	virtual void Render(const RenderCamera& camera, const RenderEngine::SceneData& renderData) = 0;

	virtual U32 GetPriority() = 0;
};
