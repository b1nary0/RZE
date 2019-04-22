#pragma once

namespace Diotima
{
	class GFXRenderPass;

	// GFXPassGraph is the systemic implementation of GFXRenderPass.
	// It provides the API to compile, prepare, and execute the frame graph,
	// ?? ultimately culminating in the final frame output ??
	class GFXPassGraph
	{
		// #TODO(Back this class up with a linear allocator for the nodes to keep things quick)

	private:
		struct GraphNode
		{
			GraphNode* Prev;
			GraphNode* Next;

			GFXRenderPass* RenderPass;
		};

	public:
		GFXPassGraph() = default;
		~GFXPassGraph() = default;

		// #TODO(Execute will set up the frame environment and 
		//       should return the final render target, the result of n GFXRenderPasses.)
		void Execute();

	private:
		// #TODO(Josh::Need a resource tracking/managing backing for the input/output resources
		//             which implies that we need to represent the concept of a GPU resource as
		//             it relates to a GFXPassGraph context. E.g create and track a GPU resource
		//             in a frame arena to be reconstructed each frame. Set up the resource according
		//             to the GFXRenderPass' criteria.)
		GraphNode* mHead;
		GraphNode* mTail;
	};
}