#pragma once

#include <Rendering/ImGui/ImGuiRenderer.h>

namespace Rendering
{
	class DX11Device;

	// ImGui's Direct3D 11 renderer backend (imgui_impl_dx11), drawing with the device's context
	class DX11ImGuiRenderer : public IImGuiRenderer
	{
	public:
		explicit DX11ImGuiRenderer(DX11Device& device);

	// IImGuiRenderer interface
	public:
		void Initialize() override;
		void NewFrame() override;
		void UpdateTexture(ImTextureData* texture) override;
		void RenderDrawData(ImDrawData* drawData) override;
		void Shutdown() override;

	private:
		DX11Device& m_device;
	};
}
