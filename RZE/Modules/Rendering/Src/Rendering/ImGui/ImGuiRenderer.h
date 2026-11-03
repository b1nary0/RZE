#pragma once

struct ImDrawData;
struct ImTextureData;

namespace Rendering
{
	// ImGui's renderer backend for the device's graphics API (GFXDeviceFactory::CreateImGuiRenderer): it draws ImGui's
	// frames with the device, beside it rather than part of it. The window's side of ImGui is Windowing/ImGuiPlatform.
	class IImGuiRenderer
	{
	public:
		IImGuiRenderer() = default;
		virtual ~IImGuiRenderer() = default;

	public:
		// Once ImGui's context exists, while the device is attached to the calling thread
		virtual void Initialize() = 0;
		// Each UI frame, on the main thread, before ImGui::NewFrame. Uses no graphics API calls.
		virtual void NewFrame() = 0;
		// Makes, updates or destroys a texture ImGui asks for (e.g. its font atlas), on the render thread
		virtual void UpdateTexture(ImTextureData* texture) = 0;
		// On the render thread
		virtual void RenderDrawData(ImDrawData* drawData) = 0;
		// Before ImGui's context is destroyed, while the device is attached to the calling thread
		virtual void Shutdown() = 0;
	};
}
