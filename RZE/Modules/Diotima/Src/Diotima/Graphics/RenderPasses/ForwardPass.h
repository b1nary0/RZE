#pragma once

#include <Diotima/Graphics/RenderPass.h>

#include <Diotima/Renderer.h>

struct ID3D11InputLayout;
struct ID3D10Blob;
struct ID3D11PixelShader;
struct ID3D11VertexShader;

namespace Diotima
{
	class DX11GFXDevice;

	class ForwardPass final : public GFXRenderPass
	{
	public:
		// #TODO(device parameter should eventually be a context (cmd list of its own)
		ForwardPass();
		virtual ~ForwardPass();

	public:
		void Initialize(int width, int height) override;
		void Execute() override;

		void OnWindowResize(int newWidth, int newHeight) override;

	public:
		// #TODO(Temp. Final architecture won't need this)
		void SetRenderer(Renderer* renderer);
		void SetDevice(DX11GFXDevice* device);

		void PrepareLights(const std::vector<Renderer::LightItemProtocol>& lights);

	private:
		void Begin();
		void End();

	private:
		DX11GFXDevice* mDevice;
		Renderer* mRenderer;

		ID3D10Blob* mVSBlob;
		ID3D10Blob* mPSBlob;
		ID3D11VertexShader* mVertexShader;
		ID3D11PixelShader* mPixelShader;

		ID3D11InputLayout* mVertexLayout;

		U32 mViewProjBuf;
		U32 mLightBuf;
		U32 mCameraDataBuf;
	};

}