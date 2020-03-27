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
		void Initialize() override;
		void Present() override;
		void SetWindow(void* windowHandle) override;
		void Shutdown() override;

	public:
		U32 CreateVertexBuffer(void* data, size_t size, U32 count) override;
		U32 CreateIndexBuffer(void* data, size_t size, U32 count) override;
		U32 CreateTextureBuffer2D(void* data, U32 width, U32 height) override;
		U32 CreateConstantBuffer(size_t memberSize, U32 maxMembers) override;

	public:
		ID3D11Device& GetHardwareDevice();
		ID3D11DeviceContext& GetDeviceContext();

		U32 GetVertexBufferCount() const;
		U32 GetIndexBufferCount() const;
		U32 GetConstantBufferCount() const;
		U32 GetTextureBufferCount() const;

		DX11GFXVertexBuffer* GetVertexBuffer(U32 bufferID);
		DX11GFXIndexBuffer* GetIndexBuffer(U32 bufferID);
		DX11GFXConstantBuffer* GetConstantBuffer(U32 bufferID);
		DX11GFXTextureBuffer2D* GetTextureBuffer2D(U32 bufferID);

	public:
		void HandleWindowResize(const Vector2D& newSize);

	// Temp access, these will move somewhere else.
	public:
		ID3D10Blob* mVSBlob;
		ID3D10Blob* mPSBlob;

		ID3D11DepthStencilView* mDepthStencilView;
		ID3D11RenderTargetView* mRenderTargetView;

		ID3D11RasterizerState* mRasterState;

	private:
		ID3D11Device* mDevice;
		ID3D11DeviceContext* mDeviceContext;

		IDXGISwapChain* mSwapChain;

		void* mWindowHandle;

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
		ID3D11VertexShader* mVertexShader;
		ID3D11PixelShader* mPixelShader;

		ID3D11Texture2D* mDepthStencilTex;
	};
}