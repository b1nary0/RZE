#pragma once

#include <Rendering/Driver/GraphicsAPI.h>

#include <memory>

namespace Rendering
{
	class IGFXDevice;
	class IImGuiRenderer;

	// Which graphics API this build draws with, and its device. The one place that knows (RZE_GFX_DX11, RZE_GFX_OPENGL).
	namespace GFXDeviceFactory
	{
		EGraphicsAPI GetGraphicsAPI();

		std::unique_ptr<IGFXDevice> CreateGFXDevice();
		// ImGui's renderer for device, which CreateGFXDevice made
		std::unique_ptr<IImGuiRenderer> CreateImGuiRenderer(IGFXDevice& device);
	}
}
