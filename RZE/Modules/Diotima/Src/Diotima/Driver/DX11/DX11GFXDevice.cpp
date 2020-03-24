#include <Diotima/Driver/DX11/DX11GFXDevice.h>

#include <Diotima/Driver/DX11/DX11.h>

#include <Diotima/Driver/DX11/DX11GFXVertexBuffer.h>
#include <Diotima/Driver/DX11/DX11GFXIndexBuffer.h>

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
		mVertexLayout->Release();
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
		return 0;
	}

	U32 DX11GFXDevice::CreateConstantBuffer(size_t memberSize, U32 maxMembers)
	{
		return 0;
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

	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	UINT numElements = ARRAYSIZE(layout);

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

		// Vertex buffer
		{
			struct VertexData
			{
				Vector3D Position;
				Vector4D Color;
			};
			VertexData vertices[4] =
			{
				{ Vector3D(-0.5f, -0.5f, 0.5f), Vector4D(1.0f, 0.0f, 0.0f, 1.0f) },
				{ Vector3D(-0.5f, 0.5f, 0.5f), Vector4D(0.0f, 1.0f, 0.0f, 1.0f) },
				{ Vector3D(0.5f, 0.5f, 0.5f), Vector4D(0.0f, 0.0f, 1.0f, 1.0f) },
				{ Vector3D(0.5f, -0.5f, 0.5f), Vector4D(0.0f, 1.0f, 0.0f, 1.0f) }
			};

			U32 vertexBufferID = CreateVertexBuffer(vertices, sizeof(VertexData), 4);

			ID3D11Buffer* vertBuf = &GetVertexBuffer(vertexBufferID)->GetHardwareBuffer();
			UINT stride = sizeof(VertexData);
			UINT offset = 0;
			mDeviceContext->IASetVertexBuffers(0, 1, &vertBuf, &stride, &offset);

			hr = mDevice->CreateInputLayout(layout, numElements, mVSBlob->GetBufferPointer(), mVSBlob->GetBufferSize(), &mVertexLayout);

			mDeviceContext->IASetInputLayout(mVertexLayout);
			mDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		}

		// Index buffer
		{
			DWORD indices[] = {
						0, 1, 2,
						0, 2, 3,
			};

			U32 indexBufferID = CreateIndexBuffer(indices, sizeof(DWORD) * 2 * 3, 1);
			ID3D11Buffer* indexBuf = &GetIndexBuffer(indexBufferID)->GetHardwareBuffer();
			mDeviceContext->IASetIndexBuffer(indexBuf, DXGI_FORMAT_R32_UINT, 0);
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
		FLOAT rgba[4] = { 0.5f, 0.5f, 1.0f, 1.0f };
		mDeviceContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
		mDeviceContext->ClearRenderTargetView(mRenderTargetView, rgba);

		mDeviceContext->DrawIndexed(6, 0, 0);

		mSwapChain->Present(0, 0);
	}

}