#pragma once

#include <Rendering/Driver/GFXDevice.h>

#include <Utils/PrimitiveDefs.h>

struct ID3D11Device;
struct ID3D11DeviceContext;
struct IDXGISwapChain2;
struct ID3D11RenderTargetView;
struct ID3D11RasterizerState;
struct ID3D11BlendState;
struct ID3D11DepthStencilState;
struct ID3D11SamplerState;

struct ID3D11DepthStencilView;
struct ID3D11Texture2D;

namespace Rendering
{
	class DX11VertexBuffer;
	class DX11IndexBuffer;
	class DX11ConstantBuffer;
	class DX11TextureBuffer2D;

	class DX11Device : public IGFXDevice
	{
	public:
		DX11Device() = default;
		~DX11Device() override = default;

	// IGFXDevice interface
	public:
		virtual void Initialize() override;
		virtual void Present() override;
		virtual void SetWindow(void* windowHandle) override;
		virtual void Shutdown() override;

	public:
		ID3D11Device& GetHardwareDevice();
		ID3D11DeviceContext& GetDeviceContext();
		
	public:
		// 0 presents at once (tearing where the system allows it), 1 on every vertical blank, 2 on every other, and so on
		void SetSyncInterval(U32 interval);
		// Signalled when the swap chain can take another frame, i.e. fewer than k_maxFrameLatency are queued. Created
		// with the swap chain and closed by Shutdown; waiting on it is safe from any thread.
		void* GetFrameLatencyWaitableObject() const { return mFrameLatencyWaitableObject; }
		//void SendTextureToBackBuffer(DX11TextureBuffer2D* texture);
		
	public:
		void HandleWindowResize(U32 newWidth, U32 newHeight);
		
	public:
		ID3D11RasterizerState* mRasterState;
		ID3D11RasterizerState* mShadowCasterRasterState;

		// EBlendState::AlphaBlend and the others; EBlendState::Opaque binds no blend state, D3D's default
		ID3D11BlendState* mAlphaBlendState;
		ID3D11BlendState* mPremultipliedBlendState;
		ID3D11BlendState* mAdditiveBlendState;
		ID3D11BlendState* mMultiplyBlendState;

		// EDepthState::ReadOnly and Off; EDepthState::ReadWrite binds no depth-stencil state, D3D's default
		// (test with LESS, write)
		ID3D11DepthStencilState* mDepthReadOnlyState;
		ID3D11DepthStencilState* mDepthOffState;

		// Depth-comparison sampler for shadow maps; bound to PS slot s1 at every Begin()
		ID3D11SamplerState* mShadowSamplerState;

		ID3D11DepthStencilView* mDepthStencilView;
		ID3D11RenderTargetView* mRenderTargetView;

		ID3D11Texture2D* mDepthStencilTex;

	private:
		// Frames that may be queued for presenting. Two, since the main thread already records a frame while the render
		// thread runs the previous one.
		static constexpr U32 k_maxFrameLatency = 2;

		void CreateSwapChain();
		void CreateBackBufferDepthStencil(U32 width, U32 height);

	private:
		IDXGISwapChain2* mSwapChain;
		ID3D11DeviceContext* mDeviceContext;
		ID3D11Device* mDevice;

		void* mWindowHandle;
		void* mFrameLatencyWaitableObject = nullptr;
		U32 mSyncInterval = 0;
		// DXGI_SWAP_CHAIN_FLAG_*, which ResizeBuffers must be given again
		U32 mSwapChainFlags = 0;
		bool mIsTearingSupported = false;
		bool mHasReportedDeviceLoss = false;
	};
}