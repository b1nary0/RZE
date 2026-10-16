#include <Rendering/Driver/DX11/DX11Device.h>

#include <Rendering/Driver/DX11/DX11.h>

#include <Utils/DebugUtils/Debug.h>
#include <Utils/Math/Vector2D.h>

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

	void DX11Device::Initialize()
	{
		// @TODO Need to move a bunch of this code out into RenderEngine
		// and then do these operations via Rendering::Renderer calls. Really the only thing
		// left for the device to do essentially is to create the swapchain and create a device, i think.
		HRESULT hr;

		D3D11_CREATE_DEVICE_FLAG deviceType = D3D11_CREATE_DEVICE_SINGLETHREADED;
#ifdef _DEBUG
		deviceType = D3D11_CREATE_DEVICE_DEBUG;
#endif

		hr = D3D11CreateDevice(
			NULL,
			D3D_DRIVER_TYPE_HARDWARE,
			NULL,
			deviceType,
			NULL,
			0,
			D3D11_SDK_VERSION,
			&mDevice,
			NULL,
			&mDeviceContext);
		AssertExpr(hr == S_OK);

		CreateSwapChain();

		ID3D11Texture2D* backBufferTexture;
		hr = mSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBufferTexture);
		AssertExpr(hr == S_OK);

		hr = mDevice->CreateRenderTargetView(backBufferTexture, NULL, &mRenderTargetView);
		AssertExpr(hr == S_OK);

		backBufferTexture->Release();

		CreateBackBufferDepthStencil(kInitialWidth, kInitialHeight);
		mDeviceContext->OMSetRenderTargets(1, &mRenderTargetView, mDepthStencilView);

		// Raster state
		{
			D3D11_RASTERIZER_DESC rasterDesc;
			ZeroMemory(&rasterDesc, sizeof(rasterDesc));
			rasterDesc.FillMode = D3D11_FILL_SOLID;
			rasterDesc.CullMode = D3D11_CULL_BACK;
			// The engine is right-handed and imports geometry as authored, with counter-clockwise front faces
			rasterDesc.FrontCounterClockwise = TRUE;
			rasterDesc.MultisampleEnable = true;

			mDevice->CreateRasterizerState(&rasterDesc, &mRasterState);
		}

		// Shadow caster raster state
		{
			D3D11_RASTERIZER_DESC rasterDesc;
			ZeroMemory(&rasterDesc, sizeof(rasterDesc));
			rasterDesc.FillMode = D3D11_FILL_SOLID;
			// Single-sided geometry facing away from the light must still cast
			rasterDesc.CullMode = D3D11_CULL_NONE;
			// Pushes stored depth away from the light, more on surfaces at grazing angles, to prevent shadow acne
			rasterDesc.DepthBias = 1000;
			rasterDesc.SlopeScaledDepthBias = 2.0f;
			rasterDesc.DepthBiasClamp = 0.0f;
			// DepthClipEnable stays false so casters in front of the light's near plane are clamped rather than clipped

			mDevice->CreateRasterizerState(&rasterDesc, &mShadowCasterRasterState);
		}

		// Alpha blend state
		{
			D3D11_BLEND_DESC blendDesc;
			ZeroMemory(&blendDesc, sizeof(blendDesc));
			blendDesc.RenderTarget[0].BlendEnable = TRUE;
			blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
			blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
			blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
			// Leaves the target's alpha as it was
			blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
			blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
			blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
			blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

			mDevice->CreateBlendState(&blendDesc, &mAlphaBlendState);

			// The rest differ only in how colour combines; alpha stays as above
			blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
			blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
			mDevice->CreateBlendState(&blendDesc, &mPremultipliedBlendState);

			blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
			blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
			mDevice->CreateBlendState(&blendDesc, &mAdditiveBlendState);

			blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_DEST_COLOR;
			blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
			mDevice->CreateBlendState(&blendDesc, &mMultiplyBlendState);
		}

		// Depth states (ReadWrite is D3D's default, so it has none)
		{
			D3D11_DEPTH_STENCIL_DESC depthDesc;
			ZeroMemory(&depthDesc, sizeof(depthDesc));
			depthDesc.DepthEnable = TRUE;
			depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
			// The same test as the default, so read-only draws agree with what opaque ones wrote
			depthDesc.DepthFunc = D3D11_COMPARISON_LESS;
			mDevice->CreateDepthStencilState(&depthDesc, &mDepthReadOnlyState);

			depthDesc.DepthEnable = FALSE;
			mDevice->CreateDepthStencilState(&depthDesc, &mDepthOffState);
		}

		// Shadow comparison sampler
		{
			D3D11_SAMPLER_DESC samplerDesc;
			ZeroMemory(&samplerDesc, sizeof(samplerDesc));
			samplerDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
			samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
			samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
			samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
			// Outside the shadow map counts as lit
			samplerDesc.BorderColor[0] = 1.0f;
			samplerDesc.BorderColor[1] = 1.0f;
			samplerDesc.BorderColor[2] = 1.0f;
			samplerDesc.BorderColor[3] = 1.0f;
			samplerDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
			samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

			mDevice->CreateSamplerState(&samplerDesc, &mShadowSamplerState);
		}
	}

	void DX11Device::CreateSwapChain()
	{
		// The swap chain has to come from the factory that made the device
		IDXGIDevice* dxgiDevice = nullptr;
		HRESULT hr = mDevice->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgiDevice));
		AssertExpr(hr == S_OK);

		IDXGIAdapter* adapter = nullptr;
		hr = dxgiDevice->GetAdapter(&adapter);
		AssertExpr(hr == S_OK);

		IDXGIFactory2* factory = nullptr;
		hr = adapter->GetParent(__uuidof(IDXGIFactory2), reinterpret_cast<void**>(&factory));
		AssertExpr(hr == S_OK);

		// Tearing lets an unsynced present show at once rather than at the next refresh; variable refresh displays need it too
		IDXGIFactory5* factory5 = nullptr;
		if (SUCCEEDED(factory->QueryInterface(__uuidof(IDXGIFactory5), reinterpret_cast<void**>(&factory5))))
		{
			BOOL allowTearing = FALSE;
			mIsTearingSupported = SUCCEEDED(factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing)))
				&& allowTearing == TRUE;
			factory5->Release();
		}

		mSwapChainFlags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
		if (mIsTearingSupported)
		{
			mSwapChainFlags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
		}

		// Flip model, for lower latency and real vsync control in a window. It can't be multisampled, and nothing needs it
		// to be: the scene renders into its own targets, and only the final blit and the editor's UI draw to the back buffer.
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
		swapChainDesc.Width = kInitialWidth;
		swapChainDesc.Height = kInitialHeight;
		swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount = 2;
		swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
		swapChainDesc.Flags = mSwapChainFlags;

		IDXGISwapChain1* swapChain = nullptr;
		hr = factory->CreateSwapChainForHwnd(mDevice, reinterpret_cast<HWND>(mWindowHandle), &swapChainDesc, nullptr, nullptr, &swapChain);
		AssertExpr(hr == S_OK);

		hr = swapChain->QueryInterface(__uuidof(IDXGISwapChain2), reinterpret_cast<void**>(&mSwapChain));
		AssertExpr(hr == S_OK);
		swapChain->Release();

		hr = mSwapChain->SetMaximumFrameLatency(k_maxFrameLatency);
		AssertExpr(hr == S_OK);
		mFrameLatencyWaitableObject = mSwapChain->GetFrameLatencyWaitableObject();

		factory->Release();
		adapter->Release();
		dxgiDevice->Release();
	}

	void DX11Device::CreateBackBufferDepthStencil(U32 width, U32 height)
	{
		D3D11_TEXTURE2D_DESC depthStencilDesc;
		ZeroMemory(&depthStencilDesc, sizeof(depthStencilDesc));

		depthStencilDesc.Width = width;
		depthStencilDesc.Height = height;
		depthStencilDesc.MipLevels = 1;
		depthStencilDesc.ArraySize = 1;
		depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		// Matches the back buffer, which flip model keeps at one sample
		depthStencilDesc.SampleDesc.Count = 1;
		depthStencilDesc.SampleDesc.Quality = 0;
		depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
		depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		depthStencilDesc.CPUAccessFlags = 0;
		depthStencilDesc.MiscFlags = 0;

		mDevice->CreateTexture2D(&depthStencilDesc, NULL, &mDepthStencilTex);
		mDevice->CreateDepthStencilView(mDepthStencilTex, NULL, &mDepthStencilView);
	}

	void DX11Device::SetWindow(void* windowHandle)
	{
		mWindowHandle = windowHandle;
	}

	void DX11Device::Shutdown()
	{
		if (mFrameLatencyWaitableObject != nullptr)
		{
			CloseHandle(mFrameLatencyWaitableObject);
			mFrameLatencyWaitableObject = nullptr;
		}

		// Before the swap chain, whose back buffer it holds
		mRenderTargetView->Release();
		mSwapChain->Release();
		mDeviceContext->Release();
		mDevice->Release();

		mDepthStencilView->Release();
		mDepthStencilTex->Release();

		mRasterState->Release();
		mShadowCasterRasterState->Release();
		mAlphaBlendState->Release();
		mPremultipliedBlendState->Release();
		mAdditiveBlendState->Release();
		mMultiplyBlendState->Release();
		mDepthReadOnlyState->Release();
		mDepthOffState->Release();
		mShadowSamplerState->Release();
	}

	void DX11Device::SetSyncInterval(U32 interval)
	{
		mSyncInterval = interval;
	}

// 	void DX11Device::SendTextureToBackBuffer(DX11TextureBuffer2D* texture)
// 	{
// 		FLOAT rgba[4] = { 1.0f, 1.0f, 0.0f, 1.0f };
// 		mDeviceContext->OMSetRenderTargets(1, &mRenderTargetView, mDepthStencilView);
// 		mDeviceContext->ClearRenderTargetView(mRenderTargetView, rgba);
// 		mDeviceContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
// 
// 		mDeviceContext->VSSetShader(mVSRenderTarget, 0, 0);
// 		mDeviceContext->PSSetShader(mPSRenderTarget, 0, 0);
// 
// 		DX11VertexBuffer* vb = GetVertexBuffer(mRenderTargetVB);
// 		ID3D11Buffer* hwvb = &vb->GetHardwareBuffer();1
// 
// 		UINT stride = sizeof(float) * 5;
// 		UINT offset = 0;
// 		mDeviceContext->IASetVertexBuffers(0, 1, &hwvb, &stride, &offset);
// 
// 		DX11IndexBuffer* ib = GetIndexBuffer(mRenderTargetIB);
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

	void DX11Device::HandleWindowResize(U32 newWidth, U32 newHeight)
	{
		// @TODO The code in here should eventually move out to RenderEngine
		// via calling the requisite Renderering::Renderer functions to do the operations
		// within. It will store an abstraction of RenderTarget.

		GetDeviceContext().OMSetRenderTargets(0, 0, 0);
		mRenderTargetView->Release();
		// Flip model can only resize once nothing references the buffers, and D3D11 destroys views lazily
		GetDeviceContext().Flush();

		// With the flags the swap chain was created with, which flip model requires
		HRESULT hr = mSwapChain->ResizeBuffers(0, newWidth, newHeight, DXGI_FORMAT_UNKNOWN, mSwapChainFlags);
		AssertExpr(hr == S_OK);

		ID3D11Texture2D* pBuffer;
		hr = mSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBuffer);
		hr = mDevice->CreateRenderTargetView(pBuffer, NULL, &mRenderTargetView);
		pBuffer->Release();

		mDepthStencilView->Release();
		mDepthStencilTex->Release();

		CreateBackBufferDepthStencil(newWidth, newHeight);
	}
	
	ID3D11Device& DX11Device::GetHardwareDevice()
	{
		AssertNotNull(mDevice);
		return *mDevice;
	}

	ID3D11DeviceContext& DX11Device::GetDeviceContext()
	{
		AssertNotNull(mDeviceContext);
		return *mDeviceContext;
	}
	
	void DX11Device::Present()
	{
		// Tearing is only allowed for an unsynced present
		const UINT flags = mSyncInterval == 0 && mIsTearingSupported ? DXGI_PRESENT_ALLOW_TEARING : 0;
		const HRESULT hr = mSwapChain->Present(mSyncInterval, flags);

		if ((hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) && !mHasReportedDeviceLoss)
		{
			mHasReportedDeviceLoss = true;
			RZE_LOG_ARGS("Present failed: the D3D11 device was %s (reason 0x%08X).", hr == DXGI_ERROR_DEVICE_REMOVED ? "removed" : "reset",
				static_cast<unsigned int>(mDevice->GetDeviceRemovedReason()));
		}
	}

}