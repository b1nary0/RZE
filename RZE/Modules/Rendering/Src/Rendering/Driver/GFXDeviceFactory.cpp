#include <Rendering/Driver/GFXDeviceFactory.h>

#include <Rendering/Driver/GFXDevice.h>
#include <Rendering/ImGui/ImGuiRenderer.h>

#if defined(RZE_GFX_DX11)
#include <Rendering/Driver/DX11/DX11Device.h>
#include <Rendering/ImGui/DX11/DX11ImGuiRenderer.h>
#elif defined(RZE_GFX_OPENGL)
#include <Rendering/Driver/OpenGL/GLDevice.h>
#include <Rendering/ImGui/OpenGL/GLImGuiRenderer.h>
#else
#error "No graphics API: define RZE_GFX_DX11 or RZE_GFX_OPENGL (BaseProject.sharpmake.cs)"
#endif

namespace Rendering
{
	namespace GFXDeviceFactory
	{
#if defined(RZE_GFX_DX11)
		EGraphicsAPI GetGraphicsAPI()
		{
			return EGraphicsAPI::DX11;
		}

		std::unique_ptr<IGFXDevice> CreateGFXDevice()
		{
			return std::make_unique<DX11Device>();
		}

		std::unique_ptr<IImGuiRenderer> CreateImGuiRenderer(IGFXDevice& device)
		{
			// Only ever given the device CreateGFXDevice made
			return std::make_unique<DX11ImGuiRenderer>(static_cast<DX11Device&>(device));
		}
#elif defined(RZE_GFX_OPENGL)
		EGraphicsAPI GetGraphicsAPI()
		{
			return EGraphicsAPI::OpenGL;
		}

		std::unique_ptr<IGFXDevice> CreateGFXDevice()
		{
			return std::make_unique<GLDevice>();
		}

		std::unique_ptr<IImGuiRenderer> CreateImGuiRenderer(IGFXDevice& /*device*/)
		{
			// It only needs the device's context to be current
			return std::make_unique<GLImGuiRenderer>();
		}
#endif
	}
}
