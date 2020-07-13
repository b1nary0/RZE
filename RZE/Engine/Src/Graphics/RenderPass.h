#pragma once

#include <Utils/PrimitiveDefs.h>

#include <vector>

class RenderPassInput
{
	std::vector<U32> BufferIDs;
};

// We could output to multiple buffers e.g gbuffer
class RenderPassOutput
{
	std::vector<U32> BufferIDs;
};

// GFXRenderPass describes how a scene or subset of the scene will be drawn
// Takes as input either nothing or the result of a previous render pass.
// It will be expected that whoever defines the render pass understands the inputs
// and outputs necessary for the frame graph.
// A GFXRenderPass should be created for all rendering operations that culminate in
// an output.
//
//
//                                                 [DEPTH BUFFER]\
//                                                 [   GBUFFER  ] \
//       Depth Pass [DEPTH BUFFER] -> GBuffer Pass [   GBUFFER  ]  -> Lighting Pass [LIGHTING] -> BackBuffer
//                                                 [   GBUFFER  ]/
//
//
class RenderPass
{
public:
	RenderPass() = default;
	virtual ~RenderPass() = default;

public:
	virtual void Initialize(int width, int height) = 0;
	virtual void Execute() = 0;

	virtual void OnWindowResize(int newWidth, int newHeight) = 0;

private:
	// #TODO
	// These ideas still need iterating
	RenderPassInput* mInput;
	RenderPassOutput* mOutput;
};