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
		//void SendTextureToBackBuffer(DX11GFXTextureBuffer2D* texture);
		
	public:
		void HandleWindowResize(U32 newWidth, U32 newHeight);

	// Temp access, these will move somewhere else.
	public:
		ID3D11RasterizerState* mRasterState;

		ID3D11DepthStencilView* mDepthStencilView;
		ID3D11RenderTargetView* mRenderTargetView;

		ID3D11Texture2D* mDepthStencilTex;

	private:
		IDXGISwapChain* mSwapChain;
		ID3D11DeviceContext* mDeviceContext;
		ID3D11Device* mDevice;
		
		void* mWindowHandle;
		U32 mSyncInterval = 0;
	};
}