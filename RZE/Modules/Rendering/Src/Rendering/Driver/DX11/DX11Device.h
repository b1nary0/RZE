#pragma once

#include <Rendering/Driver/GFXDevice.h>
#include <Rendering/RenderingDefines.h>

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
		void SetWindow(void* windowHandle) override;
		void Initialize() override;
		void Shutdown() override;

		// D3D11 isn't tied to a thread
		void AttachToCurrentThread() override {}
		void DetachFromCurrentThread() override {}

		std::shared_ptr<IVertexBuffer> CreateVertexBuffer() override;
		std::shared_ptr<IIndexBuffer> CreateIndexBuffer() override;
		std::shared_ptr<IConstantBuffer> CreateConstantBuffer(U32 alignment) override;
		std::shared_ptr<ITextureBuffer2D> CreateTextureBuffer2D() override;
		std::shared_ptr<IVertexShader> CreateVertexShader() override;
		std::shared_ptr<IPixelShader> CreatePixelShader() override;
		std::shared_ptr<IComputeShader> CreateComputeShader() override;

		void Present() override;
		void HandleWindowResize(U32 newWidth, U32 newHeight) override;
		void SetSyncInterval(U32 interval) override;
		bool WaitForPresentSlot(U32 timeoutMS) override;
		void BeginDrawSet() override;

		bool IsProfilerAttached() override;
		void BeginProfilerEvent(const char* name) override;
		void EndProfilerEvent() override;

		void ClearDepthStencilBuffer(ITextureBuffer2D& depthTexture, bool everySlice, U32 slice) override;
		void ClearRenderTargets() override;
		void SetRenderTargetBackBuffer() override;
		void SetDepthTarget(ITextureBuffer2D& depthTexture, U32 slice) override;
		void SetColourTarget(ITextureBuffer2D& colourTarget, ITextureBuffer2D* depthTexture) override;
		void GenerateMips(ITextureBuffer2D& texture) override;
		void CopyTexture(ITextureBuffer2D& source, ITextureBuffer2D& destination) override;
		void CopyTextureRegion(ITextureBuffer2D& source, U32 sourceX, U32 sourceY, U32 width, U32 height, ITextureBuffer2D& destination, U32 destinationX, U32 destinationY) override;
		void ClearRenderTarget(ITextureBuffer2D& renderTarget, const Vector4D& colour) override;

		void SetViewport(const ViewportParams& params) override;
		void SetPrimitiveTopology(EPrimitiveTopology topology) override;
		void SetRasterizerState(ERasterizerState state) override;
		void SetBlendState(EBlendState state) override;
		void SetDepthState(EDepthState state) override;
		void UnsetPixelShader() override;
		void UnsetTextureResource(U32 textureSlot) override;
		void UnsetTextureResourceCS(U32 textureSlot) override;
		void SetUnorderedAccess(ITextureBuffer2D& texture, U32 slot, U32 mip) override;
		void UnsetUnorderedAccess(U32 slot) override;

		void Draw(U32 vertexCount) override;
		void DrawIndexed(U32 indexCount) override;
		void Dispatch(U32 groupsX, U32 groupsY, U32 groupsZ) override;

		void ReadTexel(ITextureBuffer2D& texture, U32 x, U32 y, TexelReadback& result) override;
		void ReadTexture(ITextureBuffer2D& texture, TextureReadback& result) override;

	public:
		// For this backend's resources and ImGui renderer
		ID3D11Device& GetHardwareDevice();
		ID3D11DeviceContext& GetDeviceContext();

	private:
		// The state each enum asks for; null for the one D3D draws with when none is bound (EBlendState::Opaque,
		// EDepthState::ReadWrite)
		ID3D11RasterizerState* GetRasterizerState(ERasterizerState state) const;
		ID3D11BlendState* GetBlendState(EBlendState state) const;
		ID3D11DepthStencilState* GetDepthState(EDepthState state) const;

	private:
		ID3D11RasterizerState* mRasterState;
		ID3D11RasterizerState* mShadowCasterRasterState;
		ID3D11RasterizerState* mNoCullRasterState;

		// EBlendState::AlphaBlend and the others; EBlendState::Opaque binds no blend state, D3D's default
		ID3D11BlendState* mAlphaBlendState;
		ID3D11BlendState* mPremultipliedBlendState;
		ID3D11BlendState* mAdditiveBlendState;
		ID3D11BlendState* mMultiplyBlendState;

		// EDepthState::ReadOnly and Off; EDepthState::ReadWrite binds no depth-stencil state, D3D's default
		// (test with LESS, write)
		ID3D11DepthStencilState* mDepthReadOnlyState;
		ID3D11DepthStencilState* mDepthReadOnlyLessEqualState;
		ID3D11DepthStencilState* mDepthOffState;

		// Depth-comparison sampler for shadow maps; bound to PS and CS slot s1 at every Begin()
		ID3D11SamplerState* mShadowSamplerState;
		// Linear filtering that clamps at the edges, for screen-sized textures and probe data; bound to PS and CS slot s2
		// at every Begin()
		ID3D11SamplerState* mLinearClampSamplerState;

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
		// Signalled when the swap chain can take another frame, i.e. fewer than k_maxFrameLatency are queued. Created
		// with the swap chain and closed by Shutdown; waiting on it is safe from any thread.
		void* mFrameLatencyWaitableObject = nullptr;
		U32 mSyncInterval = 0;
		// DXGI_SWAP_CHAIN_FLAG_*, which ResizeBuffers must be given again
		U32 mSwapChainFlags = 0;
		bool mIsTearingSupported = false;
		bool mHasReportedDeviceLoss = false;
	};
}