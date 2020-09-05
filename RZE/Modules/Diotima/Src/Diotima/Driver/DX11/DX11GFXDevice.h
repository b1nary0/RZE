#pragma once

#include <Diotima/Driver/GFXDevice.h>

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

class Vector2D;

namespace Diotima
{
	class DX11GFXVertexBuffer;
	class DX11GFXIndexBuffer;
	class DX11GFXConstantBuffer;
	class DX11GFXTextureBuffer2D;

	class DX11GFXDevice : public IGFXDevice
	{
	public:
		DX11GFXDevice() = default;
		~DX11GFXDevice() = default;

	// IGFXDevice interface
	public:
		virtual void Initialize() override;
		virtual void Present() override;
		virtual void SetWindow(void* windowHandle) override;
		virtual void Shutdown() override;

	public:
		Int32 CreateVertexBuffer(void* data, size_t size, U32 count) override;
		Int32 CreateIndexBuffer(void* data, size_t size, U32 count) override;
		Int32 CreateTextureBuffer2D(void* data, const GFXTextureBufferParams& params) override;
		Int32 CreateRenderTarget2D(U32 width, U32 height);
		Int32 CreateConstantBuffer(size_t memberSize, U32 maxMembers) override;

	public:
		ID3D11Device& GetHardwareDevice();
		ID3D11DeviceContext& GetDeviceContext();

		U32 GetVertexBufferCount() const;
		U32 GetIndexBufferCount() const;
		U32 GetConstantBufferCount() const;
		U32 GetTextureBufferCount() const;

		DX11GFXVertexBuffer* GetVertexBuffer(Int32 bufferID);
		DX11GFXIndexBuffer* GetIndexBuffer(Int32 bufferID);
		DX11GFXConstantBuffer* GetConstantBuffer(Int32 bufferID);
		DX11GFXTextureBuffer2D* GetTextureBuffer2D(Int32 bufferID);

	public:
		void SetSyncInterval(U32 interval);
		void SendTextureToBackBuffer(DX11GFXTextureBuffer2D* texture);

	public:
		void HandleWindowResize(const Vector2D& newSize);

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
		std::vector<std::unique_ptr<DX11GFXVertexBuffer>> mVertexBuffers;
 		std::vector<std::unique_ptr<DX11GFXIndexBuffer>> mIndexBuffers;
 		std::vector<std::unique_ptr<DX11GFXConstantBuffer>> mConstantBuffers;
		std::vector<std::unique_ptr<DX11GFXTextureBuffer2D>> mTexture2DBuffers;

	// temp for testing will move after
	private:
		void SetupSceneStuff();

		ID3D11Buffer* mSquareVertBuf;
		ID3D11Buffer* mSquareIndexBuf;

		ID3D11Texture2D* mDepthStencilTex;
	};
}