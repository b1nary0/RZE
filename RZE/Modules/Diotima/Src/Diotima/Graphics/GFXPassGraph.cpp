#include <Diotima/Graphics/GFXPassGraph.h>

#include <Diotima/Renderer.h>
#include <Diotima/Graphics/RenderPasses/ForwardPass.h>

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

		std::unique_ptr<ForwardPass> forwardPass = std::make_unique<ForwardPass>();
		forwardPass->SetRenderer(renderer);
		forwardPass->Initialize();

		mRenderPasses.push_back(std::move(forwardPass));
	}

	void GFXPassGraph::Execute()
	{
		OPTICK_EVENT();

		AssertMsg(mRenderPasses.size() > 0, "Must always have at least one render pass.");

		for (auto& renderPass : mRenderPasses)
		{
			renderPass->Execute();
		}
	}

}