#include <StdAfx.h>

#include <Graphics/RenderPassGraph.h>

#include <Graphics/RenderPasses/ForwardPass.h>
#include <Graphics/RenderPasses/DepthPass.h>

#include <Utils/DebugUtils/Debug.h>

#include <Optick/optick.h>

RenderPassGraph::RenderPassGraph()
{
}

RenderPassGraph::~RenderPassGraph()
{
}

void RenderPassGraph::Build(int width, int height)
{
	OPTICK_EVENT();

	std::unique_ptr<DepthPass> depthPass = std::make_unique<DepthPass>();
	depthPass->Initialize(4098, 4098);

	std::unique_ptr<ForwardPass> forwardPass = std::make_unique<ForwardPass>();
	forwardPass->Initialize(width, height);

	mRenderPasses.push_back(std::move(depthPass));
	mRenderPasses.push_back(std::move(forwardPass));
}

void RenderPassGraph::Execute()
{
	OPTICK_EVENT();

	AssertMsg(mRenderPasses.size() > 0, "Must always have at least one render pass.");

	U32 previousPassOutput = std::numeric_limits<U32>::max();
	for (auto& renderPass : mRenderPasses)
	{
		renderPass->SetInputBuffer(previousPassOutput);
		renderPass->Execute();
		previousPassOutput = renderPass->GetOutputBuffer();
	}
}

void RenderPassGraph::OnWindowResize(int newWidth, int newHeight)
{
	for (std::unique_ptr<RenderPass>& pass : mRenderPasses)
	{
		pass->OnWindowResize(newWidth, newHeight);
	}
}