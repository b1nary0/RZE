#pragma once

#include <Graphics/RenderPass.h>

class ForwardPass final : public RenderPass
{
public:
	// #TODO(device parameter should eventually be a context (cmd list of its own)
	ForwardPass();
	virtual ~ForwardPass();

public:
	virtual void Initialize(int width, int height) override;
	virtual void Execute() override;
	virtual void OnWindowResize(int newWidth, int newHeight) override;
	virtual void SetInputBuffer(U32 bufferID) override;
	virtual U32 GetOutputBuffer() override { return std::numeric_limits<U32>::max(); }

private:
	void Begin();
	void End();

private:
};
