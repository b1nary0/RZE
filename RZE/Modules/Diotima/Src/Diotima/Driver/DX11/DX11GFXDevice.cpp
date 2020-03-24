#include <Diotima/Driver/DX11/DX11GFXDevice.h>

#include <Diotima/Driver/DX11/DX11.h>

#include <Utils/Conversions.h>
#include <Utils/Math/Vector3D.h>
#include <Utils/Math/Vector4D.h>
#include <Utils/Platform/FilePath.h>

namespace Diotima
{

	void DX11GFXDevice::Initialize()
	{
		const int initialWidth = 1024;
		const int initialHeight = 768;

		HRESULT hr;

		DXGI_MODE_DESC bufferDesc;
		ZeroMemory(&bufferDesc, sizeof(DXGI_MODE_DESC));

		bufferDesc.Width = initialWidth;
		bufferDesc.Height = initialHeight;
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
		mTriVertBuf->Release();
		mVertexShader->Release();
		mPixelShader->Release();
		mVSBlob->Release();
		mPSBlob->Release();
		mVertexLayout->Release();
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

	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	UINT numElements = ARRAYSIZE(layout);

	void DX11GFXDevice::SetupSceneStuff()
	{
		HRESULT hr;

		FilePath vertexShaderPath("Assets/Shaders/Vertex_d3d11.hlsl");
		FilePath pixelShaderPath("Assets/Shaders/Pixel_d3d11.hlsl");

		hr = D3DCompileFromFile(Conversions::StringToWString(vertexShaderPath.GetAbsolutePath()).c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VS", "vs_5_0", 0, 0, &mVSBlob, 0);
		hr = D3DCompileFromFile(Conversions::StringToWString(vertexShaderPath.GetAbsolutePath()).c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PS", "ps_5_0", 0, 0, &mPSBlob, 0);

		hr = mDevice->CreateVertexShader(mVSBlob->GetBufferPointer(), mVSBlob->GetBufferSize(), nullptr, &mVertexShader);
		hr = mDevice->CreatePixelShader(mPSBlob->GetBufferPointer(), mPSBlob->GetBufferSize(), nullptr, &mPixelShader);

		mDeviceContext->VSSetShader(mVertexShader, 0, 0);
		mDeviceContext->PSSetShader(mPixelShader, 0, 0);

		struct VertexData
		{
			Vector3D Position;
			Vector4D Color;
		};
		VertexData vertices[3] =
		{
			{ Vector3D(0.0f, 0.5f, 0.5f), Vector4D(1.0f, 0.0f, 0.0f, 1.0f) },
			{ Vector3D(0.5f, -0.5f, 0.5f), Vector4D(0.0f, 1.0f, 0.0f, 1.0f) },
			{ Vector3D(-0.5f, -0.5f, 0.5f), Vector4D(0.0f, 0.0f, 1.0f, 1.0f) }
		};

		D3D11_BUFFER_DESC vertexBufferDesc;
		ZeroMemory(&vertexBufferDesc, sizeof(vertexBufferDesc));

		vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
		vertexBufferDesc.ByteWidth = sizeof(VertexData) * 3;
		vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vertexBufferDesc.CPUAccessFlags = 0;
		vertexBufferDesc.MiscFlags = 0;

		D3D11_SUBRESOURCE_DATA vertexBufferData;
		ZeroMemory(&vertexBufferData, sizeof(vertexBufferData));

		vertexBufferData.pSysMem = vertices;
		hr = mDevice->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &mTriVertBuf);

		UINT stride = sizeof(VertexData);
		UINT offset = 0;
		mDeviceContext->IASetVertexBuffers(0, 1, &mTriVertBuf, &stride, &offset);

		hr = mDevice->CreateInputLayout(layout, numElements, mVSBlob->GetBufferPointer(), mVSBlob->GetBufferSize(), &mVertexLayout);

		mDeviceContext->IASetInputLayout(mVertexLayout);
		mDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		D3D11_VIEWPORT viewport;
		ZeroMemory(&viewport, sizeof(viewport));

		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.Width = 1024;
		viewport.Height = 768;

		mDeviceContext->RSSetViewports(1, &viewport);
	}

	void DX11GFXDevice::Present()
	{
		FLOAT rgba[4] = { 0.5f, 0.5f, 1.0f, 1.0f };
		mDeviceContext->ClearRenderTargetView(mRenderTargetView, rgba);

		mDeviceContext->Draw(3, 0);

		mSwapChain->Present(0, 0);
	}

}