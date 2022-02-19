#include <StdAfx.h>
#include <Graphics/RenderEngine.h>

#include <Graphics/RenderStage.h>
#include <Graphics/RenderStages/ForwardRenderStage.h>
#include <Graphics/RenderStages/FinalRenderTargetStage.h>

#include <Rendering/Renderer.h>

RenderEngine::RenderEngine()
{	
}

RenderEngine::~RenderEngine()
{
}

void RenderEngine::Initialize(void* windowHandle)
{
	Rendering::Renderer::Initialize(windowHandle);
	
	AddRenderStage<ForwardRenderStage>();
}

void RenderEngine::Update()
{
	OPTICK_EVENT();
	for (auto& pipeline : m_renderStages)
	{
		pipeline->Update(m_renderObjects);
	}
}

void RenderEngine::Render()
{
	OPTICK_EVENT();

	Rendering::Renderer::BeginFrame();

	for (auto& pipeline : m_renderStages)
	{
		pipeline->Render(m_renderObjects);
	}

	Rendering::Renderer::EndFrame();
}

void RenderEngine::Shutdown()
{
	Rendering::Renderer::Shutdown();
}

std::shared_ptr<RenderObject> RenderEngine::CreateRenderObject(const StaticMesh& staticMesh)
{
	OPTICK_EVENT();

	m_renderObjects.emplace_back(std::make_shared<RenderObject>());
	m_renderObjects.back()->SetStaticMesh(staticMesh);

	return m_renderObjects.back();
}

void RenderEngine::DestroyRenderObject(const std::shared_ptr<RenderObject>& renderObject)
{
	OPTICK_EVENT();

	auto iter = std::find_if(m_renderObjects.begin(), m_renderObjects.end(),
		[&renderObject](const std::shared_ptr<RenderObject>& other)
		{
			return renderObject == other;
		});

	if (iter != m_renderObjects.end())
	{
		if (m_renderObjects.size() > 1)
		{
			std::iter_swap(iter, std::prev(m_renderObjects.end()));	
		}

		m_renderObjects.erase(iter);

		renderObject.reset();
	}
}

void RenderEngine::ResizeCanvas(const Vector2D& newSize)
{
	m_canvasSize = newSize;
	
	ImGui::GetIO().DisplaySize.x = m_canvasSize.X();
	ImGui::GetIO().DisplaySize.y = m_canvasSize.Y();

	Rendering::Renderer::HandleWindowResize(m_canvasSize);

	LOG_CONSOLE_ARGS("New Canvas Size: %f x %f", m_canvasSize.X(), m_canvasSize.Y());
}

const Vector2D& RenderEngine::GetCanvasSize() const
{
	return m_canvasSize;
}

const Rendering::RenderTargetTexture& RenderEngine::GetRenderTarget()
{
	AssertNotNull(m_renderTarget);
	return *m_renderTarget;
}

void RenderEngine::InternalAddRenderStage(IRenderStage* pipeline)
{
	std::unique_ptr<IRenderStage> ptr = std::unique_ptr<IRenderStage>(pipeline);
	ptr->Initialize();
	m_renderStages.emplace_back(std::move(ptr));

	std::sort(m_renderStages.begin(), m_renderStages.end(),
		[](const std::unique_ptr<IRenderStage>& pipelineA, const std::unique_ptr<IRenderStage>& pipelineB)
		{
			return pipelineA->GetPriority() < pipelineB->GetPriority();
		});
}
