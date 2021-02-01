#pragma once

#include <memory>
#include <vector>

class RenderPass;
class LegacyRenderer;

// RenderPassGraph is the systemic implementation of RenderPass.
// It provides the API to compile, prepare, and execute the frame graph,
// ?? ultimately culminating in the final frame output ??
class RenderPassGraph
{
	// #TODO(Back this class up with a linear allocator for the nodes to keep things quick)
public:
	RenderPassGraph();
	~RenderPassGraph();

	void Build(int width, int height);

	// #TODO(Execute will set up the frame environment and 
	//       should return the final render target, the result of n GFXRenderPasses.)
	void Execute();

	void OnWindowResize(int newWidth, int newHeight);

private:
	std::vector<std::unique_ptr<RenderPass>> mRenderPasses;
};