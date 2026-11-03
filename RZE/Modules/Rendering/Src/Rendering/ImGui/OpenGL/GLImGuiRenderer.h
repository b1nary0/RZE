#pragma once

#include <Rendering/ImGui/ImGuiRenderer.h>

namespace Rendering
{
	// ImGui's OpenGL 3+ renderer backend (imgui_impl_opengl3), drawing into whatever framebuffer is bound, with the
	// device's context current. It loads its own GL functions.
	class GLImGuiRenderer : public IImGuiRenderer
	{
	// IImGuiRenderer interface
	public:
		void Initialize() override;
		void NewFrame() override;
		void UpdateTexture(ImTextureData* texture) override;
		void RenderDrawData(ImDrawData* drawData) override;
		void Shutdown() override;
	};
}
