#pragma once

#include <Diotima/Driver/GFXDevice.h>

struct ID3D11Device;
struct ID3D11DeviceContext;
struct IDXGISwapChain;
struct ID3D11RenderTargetView;

struct ID3D11Buffer;
struct ID3D11VertexShader;
struct ID3D11PixelShader;
struct ID3D10Blob;
struct ID3D10Blob;
struct ID3D11InputLayout;
struct ID3D11DepthStencilView;
struct ID3D11Texture2D;

namespace Diotima
{
	class DX11GFXDevice : public IGFXDevice
	{
	public:
		DX11GFXDevice() = default;
		~DX11GFXDevice() = default;

	// IGFXDevice interface
	public:
		void Initialize() override;
		void Present() override;
		void SetWindow(void* windowHandle) override;
		void Shutdown() override;

		U32 CreateVertexBuffer(void* data, U32 numElements) override;
		U32 CreateIndexBuffer(void* data, U32 numElements) override;
		U32 CreateTextureBuffer2D(void* data, U32 width, U32 height) override;
		U32 CreateConstantBuffer(size_t memberSize, U32 maxMembers) override;

	private:
		IDXGISwapChain* mSwapChain;
		ID3D11Device* mDevice;
		ID3D11DeviceContext* mDeviceContext;
		ID3D11RenderTargetView* mRenderTargetView;

		void* mWindowHandle;

	// temp for testing will move after
	private:
		void SetupSceneStuff();

		ID3D11Buffer* mSquareVertBuf;
		ID3D11Buffer* mSquareIndexBuf;
		ID3D11VertexShader* mVertexShader;
		ID3D11PixelShader* mPixelShader;
		ID3D10Blob* mVSBlob;
		ID3D10Blob* mPSBlob;
		ID3D11InputLayout* mVertexLayout;

		ID3D11DepthStencilView* mDepthStencilView;
		ID3D11Texture2D* mDepthStencilTex;
	};
}