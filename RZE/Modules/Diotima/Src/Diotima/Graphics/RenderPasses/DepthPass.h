#pragma once

#include <Diotima/Graphics/RenderPass.h>

#include <Diotima/Renderer.h>

namespace Diotima
{
	class DX11GFXDevice;

	class DepthPass final : public GFXRenderPass
	{
	public:
		// #TODO(device parameter should eventually be a context (cmd list of its own)
		DepthPass();
		virtual ~DepthPass();

	public:
		void Initialize(int width, int height) override;
		void Execute() override;

		void OnWindowResize(int newWidth, int newHeight) override {}

		// #TODO(Temp. Final architecture won't need this)
		void SetRenderer(Renderer* renderer);
		void SetDevice(DX11GFXDevice* device);

	private:
		void Begin();
		void End();

		void PrepareLights(const std::vector<Renderer::LightItemProtocol>& lights);

	private:
		DX11GFXDevice* mDevice;
		Renderer* mRenderer;

		U32 mLightConstantBuffer;
		U32 mWorldMatrixBuffer;
	};

}