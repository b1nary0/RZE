#pragma once

#include <Diotima/Graphics/RenderPass.h>

#include <Diotima/LegacyRenderer.h>

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
		virtual void Initialize(int width, int height) override;
		virtual void Execute() override;
		virtual void OnWindowResize(int newWidth, int newHeight) override;
		virtual void SetInputBuffer(U32 bufferID) override;
		virtual U32 GetOutputBuffer() override { return std::numeric_limits<U32>::max(); }

	public:
		// #TODO(Temp. Final architecture won't need this)
		void SetRenderer(LegacyRenderer* renderer);
		void SetDevice(DX11GFXDevice* device);

		void PrepareLights(const std::vector<LegacyRenderer::LightItemProtocol>& lights);

	private:
		void Begin();
		void End();

	private:
		DX11GFXDevice* mDevice;
		LegacyRenderer* mRenderer;

		ID3D10Blob* mVSBlob;
		ID3D10Blob* mPSBlob;
		ID3D11VertexShader* mVertexShader;
		ID3D11PixelShader* mPixelShader;

		ID3D11InputLayout* mVertexLayout;

		U32 mViewProjBuf;
		U32 mLightBuf;
		U32 mCameraDataBuf;

		U32 mShadowMapBufferID;
	};

}