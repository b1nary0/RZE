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

#include <ImGui/imgui.h>
#include <ImGui/imgui_impl_dx11.h>
#include <ImGui/imgui_impl_win32.h>

RenderEngine::RenderEngine()
{
	
}

RenderEngine::~RenderEngine()
{
	
}

void RenderEngine::Initialize(void* windowHandle)
{
	// Rendering::Renderer::Initialize
	Rendering::Renderer::Initialize(windowHandle);
}

void RenderEngine::Update()
{
}

void RenderEngine::Render()
{
	//m_vertexShader = RZE::GetResourceHandler().LoadResource<ShaderTechnique>(FilePath("Assets/Shaders/Vertex_NewRenderer.hlsl"));

	{
		OPTICK_EVENT("ImGui Render");
		ImGui::Render();
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	}
}

void RenderEngine::Shutdown()
{
	// Rendering::Renderer::Initialize
}

void RenderEngine::ResizeCanvas(const Vector2D& newSize)
{
}
