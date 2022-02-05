#include <StdAfx.h>
#include <Graphics/RenderEngine.h>

#include <Utils/Math/Vector4D.h>

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

	m_vertexShader = Rendering::Renderer::CreateVertexShader(FilePath("Assets/Shaders/Vertex_NewRenderer.hlsl"));
}

void RenderEngine::Update()
{
#ifdef IMGUI_ENABLED
	ImGui::ShowDemoWindow();
#endif
}

void RenderEngine::Render()
{
	Rendering::Renderer::BeginFrame();

	Rendering::Renderer::Begin();
	Rendering::Renderer::SetViewport({ m_canvasSize.X(), m_canvasSize.Y(), 0.0f, 1.0f, 0.0f, 0.0f });
	Rendering::Renderer::SetClearColour(Vector4D(0.5f, 0.5f, 1.0f, 1.0f));
	Rendering::Renderer::End();

#ifdef IMGUI_ENABLED
	{
		OPTICK_EVENT("ImGui Render");
		ImGui::Render();
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	}
#endif

	Rendering::Renderer::EndFrame();
}

void RenderEngine::Shutdown()
{
	Rendering::Renderer::Shutdown();
}

void RenderEngine::ResizeCanvas(const Vector2D& newSize)
{
	m_canvasSize = newSize;

#ifdef IMGUI_ENABLED
	ImGui::GetIO().DisplaySize.x = m_canvasSize.X();
	ImGui::GetIO().DisplaySize.y = m_canvasSize.Y();
#endif

	Rendering::Renderer::HandleWindowResize(m_canvasSize);

	LOG_CONSOLE_ARGS("New Canvas Size: %f x %f", m_canvasSize.X(), m_canvasSize.Y());
}
