#include <Rendering/ImGui/OpenGL/GLImGuiRenderer.h>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_opengl3.h>

#include <glad/gl.h>

namespace Rendering
{
	namespace
	{
		// Mirrors the draw data top to bottom within the display: done twice, it's as it was
		void MirrorVertically(ImDrawData& drawData)
		{
			const float mirror = 2.0f * drawData.DisplayPos.y + drawData.DisplaySize.y;
			for (ImDrawList* drawList : drawData.CmdLists)
			{
				for (ImDrawVert& vertex : drawList->VtxBuffer)
				{
					vertex.pos.y = mirror - vertex.pos.y;
				}
				for (ImDrawCmd& command : drawList->CmdBuffer)
				{
					const float top = command.ClipRect.y;
					command.ClipRect.y = mirror - command.ClipRect.w;
					command.ClipRect.w = mirror - top;
				}
			}
		}
	}

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
		// ImGui's backend always draws GL's way up, which is right for the back buffer (GLDevice). The engine's own
		// targets are drawn D3D's way up (an upper-left clip origin), as the game's UI overlay is, so it's mirrored
		// for those, and back: the same draw data is drawn again on a frame that doesn't update the UI.
		GLint clipOrigin = GL_LOWER_LEFT;
		glGetIntegerv(GL_CLIP_ORIGIN, &clipOrigin);
		const bool isDrawnD3DWayUp = clipOrigin == GL_UPPER_LEFT && drawData != nullptr;

		if (isDrawnD3DWayUp)
		{
			MirrorVertically(*drawData);
		}
		ImGui_ImplOpenGL3_RenderDrawData(drawData);
		if (isDrawnD3DWayUp)
		{
			MirrorVertically(*drawData);
		}
	}

	void GLImGuiRenderer::Shutdown()
	{
		ImGui_ImplOpenGL3_Shutdown();
	}
}
