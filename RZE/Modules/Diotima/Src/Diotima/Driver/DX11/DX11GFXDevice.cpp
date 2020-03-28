#include <Diotima/Driver/DX11/DX11GFXDevice.h>

#include <Diotima/Driver/DX11/DX11.h>

#include <Diotima/Driver/DX11/DX11GFXVertexBuffer.h>
#include <Diotima/Driver/DX11/DX11GFXIndexBuffer.h>
#include <Diotima/Driver/DX11/DX11GFXConstantBuffer.h>
#include <Diotima/Driver/DX11/DX11GFXTextureBuffer2D.h>

#include <Utils/Conversions.h>
#include <Utils/DebugUtils/Debug.h>
#include <Utils/Math/Vector2D.h>
#include <Utils/Platform/FilePath.h>

namespace Diotima
{
	const int kInitialWidth = 1600;
	const int kInitialHeight = 900;

	D3D11_INPUT_ELEMENT_DESC k_RTTVertLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "UV", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	UINT layoutElementCount = ARRAYSIZE(k_RTTVertLayout);

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
		swapChainDesc.SampleDesc.Count = 4;
		swapChainDesc.SampleDesc.Quality = 2;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount = 1;
		swapChainDesc.OutputWindow = reinterpret_cast<HWND>(mWindowHandle);
		swapChainDesc.Windowed = true;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

		hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, D3D11_CREATE_DEVICE_DEBUG, NULL, NULL, D3D11_SDK_VERSION, &swapChainDesc, &mSwapChain, &mDevice, NULL, &mDeviceContext);
		
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
			depthStencilDesc.SampleDesc.Count = 4;
			depthStencilDesc.SampleDesc.Quality = 2;
			depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
			depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
			depthStencilDesc.CPUAccessFlags = 0;
			depthStencilDesc.MiscFlags = 0;

			mDevice->CreateTexture2D(&depthStencilDesc, NULL, &mDepthStencilTex);
			mDevice->CreateDepthStencilView(mDepthStencilTex, NULL, &mDepthStencilView);

			mDeviceContext->OMSetRenderTargets(1, &mRenderTargetView, mDepthStencilView);
		}

		// Raster state
		{
			D3D11_RASTERIZER_DESC rasterDesc;
			ZeroMemory(&rasterDesc, sizeof(rasterDesc));
			rasterDesc.FillMode = D3D11_FILL_SOLID;
			rasterDesc.CullMode = D3D11_CULL_BACK;
			rasterDesc.MultisampleEnable = true;

			mDevice->CreateRasterizerState(&rasterDesc, &mRasterState);
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
		mDepthStencilView->Release();
		mDepthStencilTex->Release();

		for (auto& vertexBuf : mVertexBuffers)
		{
			vertexBuf->Release();
			vertexBuf.reset();
		}

		for (auto& indexBuf : mIndexBuffers)
		{
			indexBuf->Release();
			indexBuf.reset();
		}

		for (auto& constantBuf : mConstantBuffers)
		{
			constantBuf->Release();
			constantBuf.reset();
		}

		for (auto& t2dBuf : mTexture2DBuffers)
		{
			t2dBuf->Release();
			t2dBuf.reset();
		}
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

	U32 DX11GFXDevice::CreateRenderTarget2D(U32 width, U32 height)
	{
		mTexture2DBuffers.push_back(std::make_unique<DX11GFXTextureBuffer2D>());
		mTexture2DBuffers.back()->SetDevice(this);
		mTexture2DBuffers.back()->SetIsRenderTarget();
		mTexture2DBuffers.back()->Allocate(nullptr, width, height);

		return static_cast<U32>(mTexture2DBuffers.size() - 1);
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

	void DX11GFXDevice::SendTextureToBackBuffer(DX11GFXTextureBuffer2D* texture)
	{
		FLOAT rgba[4] = { 1.0f, 1.0f, 0.0f, 1.0f };
		mDeviceContext->OMSetRenderTargets(1, &mRenderTargetView, mDepthStencilView);
		mDeviceContext->ClearRenderTargetView(mRenderTargetView, rgba);
		mDeviceContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

		mDeviceContext->VSSetShader(mVSRenderTarget, 0, 0);
		mDeviceContext->PSSetShader(mPSRenderTarget, 0, 0);

		DX11GFXVertexBuffer* vb = GetVertexBuffer(mRenderTargetVB);
		ID3D11Buffer* hwvb = &vb->GetHardwareBuffer();

		UINT stride = sizeof(float) * 5;
		UINT offset = 0;
		mDeviceContext->IASetVertexBuffers(0, 1, &hwvb, &stride, &offset);

		DX11GFXIndexBuffer* ib = GetIndexBuffer(mRenderTargetIB);
		mDeviceContext->IASetIndexBuffer(&ib->GetHardwareBuffer(), DXGI_FORMAT_R32_UINT, 0);

		mDeviceContext->IASetInputLayout(mRTTVertLayout);

		ID3D11ShaderResourceView* texSRV = &texture->GetResourceView();
		ID3D11SamplerState* texSamplerState = &texture->GetSamplerState();
		mDeviceContext->PSSetShaderResources(0, 1, &texSRV);
		mDeviceContext->PSSetSamplers(0, 1, &texSamplerState);

		mDeviceContext->DrawIndexed(6, 0, 0);

		ID3D11ShaderResourceView* const pSRV[1] = { NULL };
		mDeviceContext->PSSetShaderResources(0, 1, pSRV);
	}

	void DX11GFXDevice::HandleWindowResize(const Vector2D& newSize)
	{
		GetDeviceContext().OMSetRenderTargets(0, 0, 0);
		mRenderTargetView->Release();

		UINT newWidth = static_cast<UINT>(newSize.X());
		UINT newHeight = static_cast<UINT>(newSize.Y());

		HRESULT hr = mSwapChain->ResizeBuffers(0, newWidth, newHeight, DXGI_FORMAT_UNKNOWN, 0);

		ID3D11Texture2D* pBuffer;
		hr = mSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBuffer);
		hr = mDevice->CreateRenderTargetView(pBuffer, NULL, &mRenderTargetView);
		pBuffer->Release();

		mDepthStencilView->Release();
		mDepthStencilTex->Release();

		D3D11_TEXTURE2D_DESC depthStencilDesc;
		ZeroMemory(&depthStencilDesc, sizeof(depthStencilDesc));

		depthStencilDesc.Width = newWidth;
		depthStencilDesc.Height = newHeight;
		depthStencilDesc.MipLevels = 1;
		depthStencilDesc.ArraySize = 1;
		depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		depthStencilDesc.SampleDesc.Count = 4;
		depthStencilDesc.SampleDesc.Quality = 2;
		depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
		depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		depthStencilDesc.CPUAccessFlags = 0;
		depthStencilDesc.MiscFlags = 0;

		mDevice->CreateTexture2D(&depthStencilDesc, NULL, &mDepthStencilTex);
		mDevice->CreateDepthStencilView(mDepthStencilTex, NULL, &mDepthStencilView);

		D3D11_VIEWPORT viewport;
		viewport.Width = (FLOAT)newWidth;
		viewport.Height = (FLOAT)newHeight;
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		GetDeviceContext().RSSetViewports(1, &viewport);
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
		ID3D10Blob* error;

		// RenderTarget stuff
		{
			FilePath pixelShaderPath("Assets/Shaders/RenderTargetBasic.hlsl");
			hr = D3DCompileFromFile(Conversions::StringToWString(pixelShaderPath.GetAbsolutePath()).c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VS", "vs_5_0", 0, 0, &mRenderTargetVSBlob, &error);
			hr = D3DCompileFromFile(Conversions::StringToWString(pixelShaderPath.GetAbsolutePath()).c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PS", "ps_5_0", 0, 0, &mRenderTargetPSBlob, &error);

			if (error)
			{
				char const* message =
					static_cast<char const*>(error->GetBufferPointer());

				// Write the warning to the output window when the program is
				// executing through the Microsoft Visual Studio IDE.
				size_t const length = strlen(message);
				std::wstring output = L"";
				for (size_t i = 0; i < length; ++i)
				{
					output += static_cast<wchar_t>(message[i]);
				}
				output += L'\n';
				OutputDebugString(output.c_str());
			}

			hr = mDevice->CreateVertexShader(mRenderTargetVSBlob->GetBufferPointer(), mRenderTargetVSBlob->GetBufferSize(), nullptr, &mVSRenderTarget);
			hr = mDevice->CreatePixelShader(mRenderTargetPSBlob->GetBufferPointer(), mRenderTargetPSBlob->GetBufferSize(), nullptr, &mPSRenderTarget);

			struct Vertex
			{
				float x;
				float y;
				float z;
				float u;
				float v;
			};

// 			{-1.f, 1.f, 0.f},
// 			{ -1.f,-1.f,0.f }
// 			{1.f, 1.f, 0.f},
// 			{ 1.f,-1.f,0.f },

			Vertex v[] =
			{
				{-1.0f, -1.0f, 0.0f, 0.0f, 1.0f}, // TL
				{-1.0f, 1.0f, 0.0f, 0.0f, 0.0f},  // BL
				{1.0f, 1.0f, 0.0f, 1.0f, 0.0f}, // BR
				{1.0f, -1.0f, 0.0f, 1.0f, 1.0f} // TR
			};

			DWORD indices[] = {
				0, 1, 2,
				0, 2, 3,
			};
			mRenderTargetVB = CreateVertexBuffer(v, sizeof(Vertex) * 4, 1);
			mRenderTargetIB = CreateIndexBuffer(indices, sizeof(DWORD) * 4 * 8, 1);

			hr = mDevice->CreateInputLayout(k_RTTVertLayout, layoutElementCount, mRenderTargetVSBlob->GetBufferPointer(), mRenderTargetVSBlob->GetBufferSize(), &mRTTVertLayout);

			D3D11_TEXTURE2D_DESC depthStencilDesc;
			ZeroMemory(&depthStencilDesc, sizeof(depthStencilDesc));

			depthStencilDesc.Width = 1024;
			depthStencilDesc.Height = 576;
			depthStencilDesc.MipLevels = 1;
			depthStencilDesc.ArraySize = 1;
			depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
			depthStencilDesc.SampleDesc.Count = 1;
			depthStencilDesc.SampleDesc.Quality = 0;
			depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
			depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
			depthStencilDesc.CPUAccessFlags = 0;
			depthStencilDesc.MiscFlags = 0;

			mDevice->CreateTexture2D(&depthStencilDesc, NULL, &mRenderTargetDepthTex);
			mDevice->CreateDepthStencilView(mRenderTargetDepthTex, NULL, &mRenderTargetDSV);
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