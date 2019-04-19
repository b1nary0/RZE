#pragma once

namespace Diotima
{
	// GFXPassGraph is the systemic implementation of GFXRenderPass.
	// It provides the API to compile, prepare, and execute the frame graph,
	// ?? ultimately culminating in the final frame output ??
	class GFXPassGraph
	{
	public:
		GFXPassGraph() = default;
		~GFXPassGraph() = default;

	private:
		// #TODO(Josh::Need a resource tracking/managing backing for the input/output resources
		//             which implies that we need to represent the concept of a GPU resource as
		//             it relates to a GFXPassGraph context. E.g create and track a GPU resource
		//             in a frame arena to be reconstructed each frame. Set up the resource according
		//             to the GFXRenderPass' criteria.)
	};
}