#pragma once

#include <Graphics/RenderEngine.h>

#include <Utils/PrimitiveDefs.h>

#include <memory>
#include <vector>

class RenderObject;
class LightObject;
struct RenderCamera;

struct RenderStageData
{
	RenderCamera m_camera;
	std::vector<std::unique_ptr<RenderObject>>* m_renderObjects = nullptr;
	std::vector<std::unique_ptr<LightObject>>* m_lights = nullptr;
};

class IRenderStage
{
public:
	IRenderStage() {};
	virtual ~IRenderStage() {};

public:
	virtual void Initialize() = 0;
	virtual void Update(const RenderStageData& renderData) = 0;
	virtual void Render(const RenderStageData& renderData) = 0;

	virtual U32 GetPriority() = 0;
};
