#pragma once

#include <Rendering/Driver/GFXDevice.h>

#include <Utils/PrimitiveDefs.h>

struct ID3D11Device;
struct ID3D11DeviceContext;
struct IDXGISwapChain;
struct ID3D11RenderTargetView;
struct ID3D11RasterizerState;

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
		void SetSyncInterval(U32 interval);
		//void SendTextureToBackBuffer(DX11GFXTextureBuffer2D* texture);
		
	public:
		void HandleWindowResize(U32 newWidth, U32 newHeight);
		
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