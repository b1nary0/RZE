#include <Rendering/ImGui/DX11/DX11ImGuiRenderer.h>

#include <Rendering/Driver/DX11/DX11Device.h>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_dx11.h>

namespace Rendering
{
	DX11ImGuiRenderer::DX11ImGuiRenderer(DX11Device& device)
		: m_device(device)
	{
	}

	void DX11ImGuiRenderer::Initialize()
	{
		ImGui_ImplDX11_Init(&m_device.GetHardwareDevice(), &m_device.GetDeviceContext());
		// Now, as GLImGuiRenderer does: NewFrame would make them on the main thread, while the render thread uses the
		// device, which is made single-threaded (DX11Device::Initialize). Seen deadlocked in the driver, loading
		// RenderPerfTest.scene.
		ImGui_ImplDX11_CreateDeviceObjects();
	}

	void DX11ImGuiRenderer::NewFrame()
	{
		ImGui_ImplDX11_NewFrame();
	}

	void DX11ImGuiRenderer::UpdateTexture(ImTextureData* texture)
	{
		ImGui_ImplDX11_UpdateTexture(texture);
	}

	void DX11ImGuiRenderer::RenderDrawData(ImDrawData* drawData)
	{
		ImGui_ImplDX11_RenderDrawData(drawData);
	}

	void DX11ImGuiRenderer::Shutdown()
	{
		ImGui_ImplDX11_Shutdown();
	}
}
