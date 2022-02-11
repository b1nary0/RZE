#pragma once

#include <Utils/PrimitiveDefs.h>

class RenderObject;

class IRenderPipeline
{
public:
	IRenderPipeline() {};
	virtual ~IRenderPipeline() {};

public:
	virtual void Initialize() = 0;
	virtual void Update(const std::vector<std::shared_ptr<RenderObject>>& renderObjects) = 0;
	virtual void Render(const std::vector<std::shared_ptr<RenderObject>>& renderObjects) = 0;

	virtual U32 GetPriority() = 0;
};
