#pragma once

#include <Graphics/RenderPass.h>

class DepthPass final : public RenderPass
{
public:
	// #TODO(device parameter should eventually be a context (cmd list of its own)
	DepthPass();
	virtual ~DepthPass();

public:
	virtual void Initialize(int width, int height) override;
	virtual void Execute() override;
	virtual void OnWindowResize(int newWidth, int newHeight) override {}
	virtual void SetInputBuffer(U32 bufferID) override {}

private:
	void Begin();
	void End();
};