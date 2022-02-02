#include <Rendering/Driver/DX11/DX11GFXDevice.h>

#include <Rendering/Driver/DX11/DX11.h>

#include <Utils/Conversions.h>
#include <Utils/DebugUtils/Debug.h>
#include <Utils/Math/Vector2D.h>
#include <Utils/Platform/FilePath.h>

namespace
{
	auto errorCB = [](ID3D10Blob* error)
	{
		if (error != nullptr)
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
	};
}

namespace Rendering
{
	const int kInitialWidth = 1600;
	const int kInitialHeight = 900;

	// #TODO
	// See if there's a better place for this. Just putting this here
	// out of laziness
	constexpr Int32 kInvalidBufferID = -1;

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
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount = 1;
		swapChainDesc.OutputWindow = reinterpret_cast<HWND>(mWindowHandle);
		swapChainDesc.Windowed = true;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

		hr = D3D11CreateDeviceAndSwapChain(
			NULL, 
			D3D_DRIVER_TYPE_HARDWARE, 
			NULL, 
			D3D11_CREATE_DEVICE_DEBUG, 
			NULL, 
			NULL, 
			D3D11_SDK_VERSION, 
			&swapChainDesc, 
			&mSwapChain, 
			&mDevice, 
			NULL, 
			&mDeviceContext);
		AssertExpr(hr == S_OK);
		
		ID3D11Texture2D* backBufferTexture;
		hr = mSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBufferTexture);
		AssertExpr(hr == S_OK);

		hr = mDevice->CreateRenderTargetView(backBufferTexture, NULL, &mRenderTargetView);
		AssertExpr(hr == S_OK);

		backBufferTexture->Release();

		//mDeviceContext->OMSetRenderTargets(1, &mRenderTargetView, NULL);

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
			depthStencilDesc.SampleDesc.Quality = 0;
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

		//SetupSceneStuff();
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

		mDepthStencilView->Release();
		mDepthStencilTex->Release();
		
		for (auto& pixelShader : mPixelShaders)
		{
			pixelShader->Release();
			pixelShader.reset();
		}
	}

	void DX11GFXDevice::SetSyncInterval(U32 interval)
	{
		mSyncInterval = interval;
	}

// 	void DX11GFXDevice::SendTextureToBackBuffer(DX11GFXTextureBuffer2D* texture)
// 	{
// 		FLOAT rgba[4] = { 1.0f, 1.0f, 0.0f, 1.0f };
// 		mDeviceContext->OMSetRenderTargets(1, &mRenderTargetView, mDepthStencilView);
// 		mDeviceContext->ClearRenderTargetView(mRenderTargetView, rgba);
// 		mDeviceContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
// 
// 		mDeviceContext->VSSetShader(mVSRenderTarget, 0, 0);
// 		mDeviceContext->PSSetShader(mPSRenderTarget, 0, 0);
// 
// 		DX11GFXVertexBuffer* vb = GetVertexBuffer(mRenderTargetVB);
// 		ID3D11Buffer* hwvb = &vb->GetHardwareBuffer();1
// 
// 		UINT stride = sizeof(float) * 5;
// 		UINT offset = 0;
// 		mDeviceContext->IASetVertexBuffers(0, 1, &hwvb, &stride, &offset);
// 
// 		DX11GFXIndexBuffer* ib = GetIndexBuffer(mRenderTargetIB);
// 		mDeviceContext->IASetIndexBuffer(&ib->GetHardwareBuffer(), DXGI_FORMAT_R32_UINT, 0);
// 
// 		mDeviceContext->IASetInputLayout(mRTTVertLayout);
// 
// 		ID3D11ShaderResourceView* texSRV = &texture->GetResourceView();
// 		ID3D11SamplerState* texSamplerState = &texture->GetSamplerState();
// 		mDeviceContext->PSSetShaderResources(0, 1, &texSRV);
// 		mDeviceContext->PSSetSamplers(0, 1, &texSamplerState);
// 
// 		mDeviceContext->DrawIndexed(6, 0, 0);
// 
// 		ID3D11ShaderResourceView* const pSRV[1] = { NULL };
// 		mDeviceContext->PSSetShaderResources(0, 1, pSRV);
// 	}

	void DX11GFXDevice::SetRenderTarget(ID3D11Texture2D* texture)
	{
		AssertFalse();
		// #TODO
		// This is big bug. Not disposing of old resources. Commenting for now
		// and asserting if this function is called.
		//HRESULT hr = mDevice->CreateRenderTargetView(texture, NULL, &mRenderTargetView);
	}

	void DX11GFXDevice::HandleWindowResize(U32 newWidth, U32 newHeight)
	{
		GetDeviceContext().OMSetRenderTargets(0, 0, 0);
		mRenderTargetView->Release();

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
		depthStencilDesc.SampleDesc.Quality = 0;
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
	
	void DX11GFXDevice::Present()
	{
		mSwapChain->Present(mSyncInterval, 0);
	}

	void DX11GFXDevice::ShaderDeleteWrapper::Release()
	{
		mHWShader->Release();
	}

}