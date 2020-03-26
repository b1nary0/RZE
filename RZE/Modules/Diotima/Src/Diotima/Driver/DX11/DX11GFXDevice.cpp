#include <Diotima/Driver/DX11/DX11GFXDevice.h>

#include <Diotima/Driver/DX11/DX11.h>

#include <Diotima/Driver/DX11/DX11GFXVertexBuffer.h>
#include <Diotima/Driver/DX11/DX11GFXIndexBuffer.h>
#include <Diotima/Driver/DX11/DX11GFXConstantBuffer.h>
#include <Diotima/Driver/DX11/DX11GFXTextureBuffer2D.h>

#include <Utils/Conversions.h>
#include <Utils/DebugUtils/Debug.h>
#include <Utils/Math/Vector3D.h>
#include <Utils/Math/Vector4D.h>
#include <Utils/Platform/FilePath.h>

namespace Diotima
{
	const int kInitialWidth = 1024;
	const int kInitialHeight = 768;

	void DX11GFXDevice::Initialize()
	{
		HRESULT hr;

		DXGI_MODE_DESC bufferDesc;
		ZeroMemory(&bufferDesc, sizeof(DXGI_MODE_DESC));

		bufferDesc.Width = kInitialWidth;
		bufferDesc.Height = kInitialHeight;
		bufferDesc.RefreshRate.Denominator = 60;
		bufferDesc.RefreshRate.Numerator = 1;
		bufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		bufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		bufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

		DXGI_SWAP_CHAIN_DESC swapChainDesc;
		ZeroMemory(&swapChainDesc, sizeof(DXGI_SWAP_CHAIN_DESC));

		swapChainDesc.BufferDesc = bufferDesc;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount = 1;
		swapChainDesc.OutputWindow = reinterpret_cast<HWND>(mWindowHandle);
		swapChainDesc.Windowed = true;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

		hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, NULL, NULL, NULL, D3D11_SDK_VERSION, &swapChainDesc, &mSwapChain, &mDevice, NULL, &mDeviceContext);
		
		ID3D11Texture2D* backBufferTexture;
		hr = mSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBufferTexture);

		hr = mDevice->CreateRenderTargetView(backBufferTexture, NULL, &mRenderTargetView);
		backBufferTexture->Release();

		mDeviceContext->OMSetRenderTargets(1, &mRenderTargetView, NULL);

		// DepthStencil
		{
			D3D11_TEXTURE2D_DESC depthStencilDesc;
			ZeroMemory(&depthStencilDesc, sizeof(depthStencilDesc));

			depthStencilDesc.Width = kInitialWidth;
			depthStencilDesc.Height = kInitialHeight;
			depthStencilDesc.MipLevels = 1;
			depthStencilDesc.ArraySize = 1;
			depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
			depthStencilDesc.SampleDesc.Count = 1;
			depthStencilDesc.SampleDesc.Quality = 0;
			depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
			depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
			depthStencilDesc.CPUAccessFlags = 0;
			depthStencilDesc.MiscFlags = 0;

			mDevice->CreateTexture2D(&depthStencilDesc, NULL, &mDepthStencilTex);
			mDevice->CreateDepthStencilView(mDepthStencilTex, NULL, &mDepthStencilView);

			mDeviceContext->OMSetRenderTargets(1, &mRenderTargetView, mDepthStencilView);
		}

		SetupSceneStuff();
	}

	void DX11GFXDevice::SetWindow(void* windowHandle)
	{
		mWindowHandle = windowHandle;
	}

	void DX11GFXDevice::Shutdown()
	{
		mSwapChain->Release();
		mDevice->Release();
		mDeviceContext->Release();

		// temp remove later
		mSquareVertBuf->Release();
		mSquareIndexBuf->Release();
		mVertexShader->Release();
		mPixelShader->Release();
		mVSBlob->Release();
		mPSBlob->Release();
		mDepthStencilView->Release();
		mDepthStencilTex->Release();
	}

	U32 DX11GFXDevice::CreateVertexBuffer(void* data, size_t size, U32 count)
	{
		mVertexBuffers.push_back(std::make_unique<DX11GFXVertexBuffer>());
		mVertexBuffers.back()->SetDevice(this);
		mVertexBuffers.back()->Allocate(data, size, count);

		return static_cast<U32>(mVertexBuffers.size() - 1);
	}

	U32 DX11GFXDevice::CreateIndexBuffer(void* data, size_t size, U32 count)
	{
		mIndexBuffers.push_back(std::make_unique<DX11GFXIndexBuffer>());
		mIndexBuffers.back()->SetDevice(this);
		mIndexBuffers.back()->Allocate(data, size, count);

		return static_cast<U32>(mIndexBuffers.size() - 1);
	}

	U32 DX11GFXDevice::CreateTextureBuffer2D(void* data, U32 width, U32 height)
	{
		mTexture2DBuffers.push_back(std::make_unique<DX11GFXTextureBuffer2D>());
		mTexture2DBuffers.back()->SetDevice(this);
		mTexture2DBuffers.back()->Allocate(data, width, height);

		return static_cast<U32>(mTexture2DBuffers.size() - 1);
	}

	U32 DX11GFXDevice::CreateConstantBuffer(size_t memberSize, U32 maxMembers)
	{
		mConstantBuffers.push_back(std::make_unique<DX11GFXConstantBuffer>());
		mConstantBuffers.back()->SetDevice(this);
		mConstantBuffers.back()->Allocate(memberSize, maxMembers);

		return static_cast<U32>(mConstantBuffers.size() - 1);
	}

	DX11GFXVertexBuffer* DX11GFXDevice::GetVertexBuffer(U32 bufferID)
	{
		AssertExpr(mVertexBuffers.size() > bufferID);
		return mVertexBuffers[bufferID].get();
	}

	Diotima::DX11GFXIndexBuffer* DX11GFXDevice::GetIndexBuffer(U32 bufferID)
	{
		AssertExpr(mIndexBuffers.size() > bufferID);
		return mIndexBuffers[bufferID].get();
	}

	Diotima::DX11GFXConstantBuffer* DX11GFXDevice::GetConstantBuffer(U32 bufferID)
	{
		AssertExpr(mConstantBuffers.size() > bufferID);
		return mConstantBuffers[bufferID].get();
	}

	Diotima::DX11GFXTextureBuffer2D* DX11GFXDevice::GetTextureBuffer2D(U32 bufferID)
	{
		AssertExpr(mTexture2DBuffers.size() > bufferID);
		return mTexture2DBuffers[bufferID].get();
	}

	U32 DX11GFXDevice::GetVertexBufferCount() const
	{
		return mVertexBuffers.size();
	}

	U32 DX11GFXDevice::GetIndexBufferCount() const
	{
		return mVertexBuffers.size();
	}

	U32 DX11GFXDevice::GetConstantBufferCount() const
	{
		return mVertexBuffers.size();
	}

	U32 DX11GFXDevice::GetTextureBufferCount() const
	{
		return mTexture2DBuffers.size();
	}

	ID3D11Device& DX11GFXDevice::GetHardwareDevice()
	{
		AssertNotNull(mDevice);
		return *mDevice;
	}

	ID3D11DeviceContext& DX11GFXDevice::GetDeviceContext()
	{
		AssertNotNull(mDeviceContext);
		return *mDeviceContext;
	}

	void DX11GFXDevice::SetupSceneStuff()
	{
		HRESULT hr;

		// Shaders
		{
			FilePath vertexShaderPath("Assets/Shaders/Vertex_d3d11.hlsl");
			FilePath pixelShaderPath("Assets/Shaders/Pixel_d3d11.hlsl");

			hr = D3DCompileFromFile(Conversions::StringToWString(vertexShaderPath.GetAbsolutePath()).c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VS", "vs_5_0", 0, 0, &mVSBlob, 0);
			hr = D3DCompileFromFile(Conversions::StringToWString(vertexShaderPath.GetAbsolutePath()).c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PS", "ps_5_0", 0, 0, &mPSBlob, 0);

			hr = mDevice->CreateVertexShader(mVSBlob->GetBufferPointer(), mVSBlob->GetBufferSize(), nullptr, &mVertexShader);
			hr = mDevice->CreatePixelShader(mPSBlob->GetBufferPointer(), mPSBlob->GetBufferSize(), nullptr, &mPixelShader);

			mDeviceContext->VSSetShader(mVertexShader, 0, 0);
			mDeviceContext->PSSetShader(mPixelShader, 0, 0);
		}

		// Viewport
		{
			D3D11_VIEWPORT viewport;
			ZeroMemory(&viewport, sizeof(viewport));

			viewport.TopLeftX = 0;
			viewport.TopLeftY = 0;
			viewport.Width = kInitialWidth;
			viewport.Height = kInitialHeight;
			viewport.MinDepth = 0.0f;
			viewport.MaxDepth = 1.0f;

			mDeviceContext->RSSetViewports(1, &viewport);
		}
	}

	void DX11GFXDevice::Present()
	{
		mSwapChain->Present(0, 0);
	}

}