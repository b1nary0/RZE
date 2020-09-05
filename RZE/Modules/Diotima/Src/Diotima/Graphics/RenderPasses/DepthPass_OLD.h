#pragma once

#include <Diotima/Graphics/RenderPass.h>

#include <Diotima/LegacyRenderer.h>

struct ID3D10Blob;
struct ID3D11VertexShader;
struct ID3D11PixelShader;
struct ID3D11InputLayout;

namespace Diotima
{
	class DX11GFXDevice;
	class DX11GFXTextureBuffer2D;

	class DepthPass_OLD final : public GFXRenderPass
	{
	public:
		// #TODO(device parameter should eventually be a context (cmd list of its own)
		DepthPass_OLD();
		virtual ~DepthPass_OLD();

	public:
		virtual void Initialize(int width, int height) override;
		virtual void Execute() override;
		virtual void OnWindowResize(int newWidth, int newHeight) override {}
		virtual void SetInputBuffer(U32 bufferID) override {}
		virtual U32 GetOutputBuffer() override;


		// #TODO(Temp. Final architecture won't need this)
		void SetRenderer(LegacyRenderer* renderer);
		void SetDevice(DX11GFXDevice* device);

	private:
		void Begin();
		void End();

		void PrepareLights(const std::vector<LegacyRenderer::LightItemProtocol>& lights);

	private:
		DX11GFXDevice* mDevice;
		LegacyRenderer* mRenderer;

		ID3D10Blob* mVSBlob;
		ID3D10Blob* mPSBlob;
		ID3D11VertexShader* mVertexShader;
		ID3D11PixelShader* mPixelShader;

		ID3D11InputLayout* mVertexLayout;

		DX11GFXTextureBuffer2D* mOutputTexture;

		U32 mOutputTextureID;
		U32 mViewProjBuf;
		U32 mLightBuf;
	};

}