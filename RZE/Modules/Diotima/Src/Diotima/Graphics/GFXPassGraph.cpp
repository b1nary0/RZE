#include <Diotima/Graphics/GFXPassGraph.h>

#include <Diotima/Renderer.h>
#include <Diotima/Graphics/RenderPasses/ForwardPass.h>
#include <Diotima/Graphics/RenderPasses/DepthPass.h>

#include <Utils/DebugUtils/Debug.h>

#include <Optick/optick.h>

namespace Diotima
{

	GFXPassGraph::GFXPassGraph()
	{
	}

	GFXPassGraph::~GFXPassGraph()
	{
	}

	void GFXPassGraph::Build(Renderer* renderer)
	{
		OPTICK_EVENT();

		int width = static_cast<int>(renderer->GetCanvasSize().X());
		int height = static_cast<int>(renderer->GetCanvasSize().Y());

 		std::unique_ptr<DepthPass> depthPass = std::make_unique<DepthPass>();
 		depthPass->SetRenderer(renderer);
 		depthPass->Initialize(4098, 4098);

		std::unique_ptr<ForwardPass> forwardPass = std::make_unique<ForwardPass>();
		forwardPass->SetRenderer(renderer);
		forwardPass->Initialize(width, height);

		mRenderPasses.push_back(std::move(depthPass));
		mRenderPasses.push_back(std::move(forwardPass));
	}

	void GFXPassGraph::Execute()
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

	void GFXPassGraph::OnWindowResize(int newWidth, int newHeight)
	{
		for (std::unique_ptr<GFXRenderPass>& pass : mRenderPasses)
		{
			pass->OnWindowResize(newWidth, newHeight);
		}
	}

}