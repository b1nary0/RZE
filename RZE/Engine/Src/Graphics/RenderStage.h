#pragma once

#include <Utils/PrimitiveDefs.h>

#include <memory>
#include <vector>

class RenderObject;

class IRenderStage
{
public:
	IRenderStage() {};
	virtual ~IRenderStage() {};

public:
	virtual void Initialize() = 0;
	virtual void Update(const std::vector<std::shared_ptr<RenderObject>>& renderObjects) = 0;
	virtual void Render(const std::vector<std::shared_ptr<RenderObject>>& renderObjects) = 0;

	virtual U32 GetPriority() = 0;
};
