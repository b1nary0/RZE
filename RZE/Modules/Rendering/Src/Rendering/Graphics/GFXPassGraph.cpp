#include <Rendering/Graphics/GFXPassGraph.h>

#include <Rendering/LegacyRenderer.h>
#if !WITH_NEW_RENDERER
#include <Rendering/Graphics/RenderPasses/ForwardPass_OLD.h>
#include <Rendering/Graphics/RenderPasses/DepthPass_OLD.h>

#include <Utils/DebugUtils/Debug.h>

#include <Optick/optick.h>

namespace Rendering
{

	GFXPassGraph::GFXPassGraph()
	{
	}

	GFXPassGraph::~GFXPassGraph()
	{
	}

	void GFXPassGraph::Build(LegacyRenderer* renderer)
	{
		OPTICK_EVENT();

		int width = static_cast<int>(renderer->GetCanvasSize().X());
		int height = static_cast<int>(renderer->GetCanvasSize().Y());

 		std::unique_ptr<DepthPass_OLD> depthPass = std::make_unique<DepthPass_OLD>();
 		depthPass->SetRenderer(renderer);
 		depthPass->Initialize(4098, 4098);

		std::unique_ptr<ForwardPass_OLD> forwardPass = std::make_unique<ForwardPass_OLD>();
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
#endif