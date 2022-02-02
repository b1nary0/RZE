#pragma once

#include <Rendering/Driver/GFXDevice.h>

#include <Utils/Platform/FilePath.h>

#include <memory>

struct ID3D11Device;
struct ID3D11DeviceContext;
struct IDXGISwapChain;
struct ID3D11RenderTargetView;
struct ID3D11RasterizerState;

struct ID3D11Buffer;
struct ID3D11VertexShader;
struct ID3D11PixelShader;
struct ID3D10Blob;
struct ID3D10Blob;
struct ID3D11InputLayout;
struct ID3D11DepthStencilView;
struct ID3D11Texture2D;

namespace Rendering
{
	class DX11VertexBuffer;
	class DX11IndexBuffer;
	class DX11GFXConstantBuffer;
	class DX11GFXTextureBuffer2D;

	class DX11Device : public IGFXDevice
	{
	public:
		DX11Device() = default;
		~DX11Device() = default;

	private:
		// #TODO
		// Temp because I couldn't be arsed writing an entire DX11Shader class
		// but unique_ptr screams with ID3D11*Shader classes
		class ShaderDeleteWrapper
		{
		public:
			ShaderDeleteWrapper() = default;
			~ShaderDeleteWrapper() = default;

		public:
			void Release();

		public:
			ID3D11PixelShader* mHWShader;
		};

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
		void SetSyncInterval(U32 interval);
		void SendTextureToBackBuffer(DX11GFXTextureBuffer2D* texture);

		void SetRenderTarget(ID3D11Texture2D* texture);

	public:
		void HandleWindowResize(U32 newWidth, U32 newHeight);

	// Temp access, these will move somewhere else.
	public:
		ID3D11DepthStencilView* mDepthStencilView;
		ID3D11RenderTargetView* mRenderTargetView;

		ID3D11RasterizerState* mRasterState;

	// Render target stuff
	private:
		ID3D10Blob* mRenderTargetPSBlob;
		ID3D10Blob* mRenderTargetVSBlob;
		ID3D11VertexShader* mVSRenderTarget;
		ID3D11PixelShader* mPSRenderTarget;
		ID3D11InputLayout* mRTTVertLayout;

		U32 mRenderTargetVB;
		U32 mRenderTargetIB;

	private:
		ID3D11Device* mDevice;
		ID3D11DeviceContext* mDeviceContext;

		IDXGISwapChain* mSwapChain;

		void* mWindowHandle;
		U32 mSyncInterval = 0;

	private:
		std::vector<std::unique_ptr<ShaderDeleteWrapper>> mPixelShaders;

	private:
		ID3D11Buffer* mSquareVertBuf;
		ID3D11Buffer* mSquareIndexBuf;

		ID3D11Texture2D* mDepthStencilTex;
	};
}