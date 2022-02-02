#include <StdAfx.h>
#include <Graphics/RenderEngine.h>

#include <EngineCore/Resources/ResourceHandler.h>

#include <Graphics/MeshGeometry.h>
#include <Graphics/Material.h>
#include <Graphics/Shader.h>
#include <Graphics/Texture2D.h>

#include <Utils/Math/Matrix4x4.h>
#include <Utils/Math/Vector2D.h>

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
	Rendering::Renderer::SetClearColour(Vector4D(1.0f, 0.5f, 0.5f, 1.0f));
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
}
