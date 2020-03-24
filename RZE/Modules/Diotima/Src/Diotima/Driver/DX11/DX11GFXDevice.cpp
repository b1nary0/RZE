#include <Diotima/Driver/DX11/DX11GFXDevice.h>

#include <Diotima/Driver/DX11/DX11.h>

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

	U32 DX11GFXDevice::CreateVertexBuffer(void* data, U32 numElements)
	{
		return 0;
	}

	U32 DX11GFXDevice::CreateIndexBuffer(void* data, U32 numElements)
	{
		return 0;
	}

	U32 DX11GFXDevice::CreateTextureBuffer2D(void* data, U32 width, U32 height)
	{
		return 0;
	}

	U32 DX11GFXDevice::CreateConstantBuffer(size_t memberSize, U32 maxMembers)
	{
		return 0;
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

		// Index buffer
		{
			DWORD indices[] = {
						0, 1, 2,
						0, 2, 3,
			};

			D3D11_BUFFER_DESC indexBufferDesc;
			ZeroMemory(&indexBufferDesc, sizeof(indexBufferDesc));

			indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
			indexBufferDesc.ByteWidth = sizeof(DWORD) * 2 * 3;
			indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
			indexBufferDesc.CPUAccessFlags = 0;
			indexBufferDesc.MiscFlags = 0;

			D3D11_SUBRESOURCE_DATA indexBufferData;
			ZeroMemory(&indexBufferData, sizeof(indexBufferData));

			indexBufferData.pSysMem = indices;
			mDevice->CreateBuffer(&indexBufferDesc, &indexBufferData, &mSquareIndexBuf);
			mDeviceContext->IASetIndexBuffer(mSquareIndexBuf, DXGI_FORMAT_R32_UINT, 0);
		}

		// Vertex buffer
		{
			D3D11_BUFFER_DESC vertexBufferDesc;
			ZeroMemory(&vertexBufferDesc, sizeof(vertexBufferDesc));

			vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
			vertexBufferDesc.ByteWidth = sizeof(VertexData) * 4;
			vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			vertexBufferDesc.CPUAccessFlags = 0;
			vertexBufferDesc.MiscFlags = 0;

			D3D11_SUBRESOURCE_DATA vertexBufferData;
			ZeroMemory(&vertexBufferData, sizeof(vertexBufferData));

			vertexBufferData.pSysMem = vertices;
			hr = mDevice->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &mSquareVertBuf);

			UINT stride = sizeof(VertexData);
			UINT offset = 0;
			mDeviceContext->IASetVertexBuffers(0, 1, &mSquareVertBuf, &stride, &offset);

			hr = mDevice->CreateInputLayout(layout, numElements, mVSBlob->GetBufferPointer(), mVSBlob->GetBufferSize(), &mVertexLayout);

			mDeviceContext->IASetInputLayout(mVertexLayout);
			mDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
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