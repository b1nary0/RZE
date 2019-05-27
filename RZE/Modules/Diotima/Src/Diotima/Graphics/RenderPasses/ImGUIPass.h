#pragma once

#include <Diotima/Renderer.h>
#include <Diotima/Graphics/RenderPass.h>

namespace Diotima
{
	class DX12GFXDevice;

	class ImGUIPass : public GFXRenderPass
	{
	public:
		ImGUIPass();
		virtual ~ImGUIPass();

	public:
		void Initialize(int width, int height) override;
		void Execute() override;

		void OnWindowResize(int newWidth, int newHeight) override;

		void SetRenderer(Renderer* renderer);
		void SetDevice(DX12GFXDevice* device);

		void Begin();
		void End();

	private:
		DX12GFXDevice* mDevice;
		Renderer* mRenderer;

		U32 mCommandList;

		ID3D12DescriptorHeap* mTextureHeap;
	};
}