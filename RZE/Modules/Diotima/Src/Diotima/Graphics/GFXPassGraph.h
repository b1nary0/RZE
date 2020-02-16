#pragma once

#include <memory>
#include <vector>

namespace Diotima
{
	class GFXRenderPass;
	class Renderer;

	// GFXPassGraph is the systemic implementation of GFXRenderPass.
	// It provides the API to compile, prepare, and execute the frame graph,
	// ?? ultimately culminating in the final frame output ??
	class GFXPassGraph
	{
		// #TODO(Back this class up with a linear allocator for the nodes to keep things quick)
	public:
		GFXPassGraph();
		~GFXPassGraph();

		void Build(Renderer* renderer);
		
		// #TODO(Execute will set up the frame environment and 
		//       should return the final render target, the result of n GFXRenderPasses.)
		void Execute();

		void OnWindowResize(int newWidth, int newHeight);

	private:
		std::vector<std::unique_ptr<GFXRenderPass>> mRenderPasses;
	};
}