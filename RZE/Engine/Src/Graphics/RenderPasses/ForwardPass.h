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

private:
	void Begin();
	void End();

private:
};
