#include <Rendering/ImGui/OpenGL/GLImGuiRenderer.h>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_opengl3.h>

namespace Rendering
{
	void GLImGuiRenderer::Initialize()
	{
		ImGui_ImplOpenGL3_Init("#version 450");
		// Now, while the context is current here: NewFrame would make them on the main thread, which has no context
		ImGui_ImplOpenGL3_CreateDeviceObjects();
	}

	void GLImGuiRenderer::NewFrame()
	{
		ImGui_ImplOpenGL3_NewFrame();
	}

	void GLImGuiRenderer::UpdateTexture(ImTextureData* texture)
	{
		ImGui_ImplOpenGL3_UpdateTexture(texture);
	}

	void GLImGuiRenderer::RenderDrawData(ImDrawData* drawData)
	{
		ImGui_ImplOpenGL3_RenderDrawData(drawData);
	}

	void GLImGuiRenderer::Shutdown()
	{
		ImGui_ImplOpenGL3_Shutdown();
	}
}
