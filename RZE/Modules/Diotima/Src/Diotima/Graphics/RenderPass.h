#pragma once

#include <Diotima/Driver/DX12/DX12.h>

#include <Utils/PrimitiveDefs.h>

#include <vector>

namespace Diotima
{
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
	class GFXRenderPass
	{
	public:
		GFXRenderPass() = default;
		virtual ~GFXRenderPass() = default;

	public:
		virtual void Initialize() = 0;
		virtual void Execute() = 0;

		virtual CD3DX12_GPU_DESCRIPTOR_HANDLE GetResourceGPUHandle(U64& handle) { return CD3DX12_GPU_DESCRIPTOR_HANDLE(); };
		virtual void SetResourceGPUHandle(CD3DX12_GPU_DESCRIPTOR_HANDLE handle) {};

	private:
		RenderPassInput* mInput;
		RenderPassOutput* mOutput;
	};
}