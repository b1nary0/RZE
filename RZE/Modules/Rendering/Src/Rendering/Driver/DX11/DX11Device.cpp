#include <Rendering/Driver/DX11/DX11Device.h>

#include <Rendering/Driver/DX11/DX11.h>
#include <Rendering/Driver/DX11/DX11ConstantBuffer.h>
#include <Rendering/Driver/DX11/DX11IndexBuffer.h>
#include <Rendering/Driver/DX11/DX11ShaderTypes.h>
#include <Rendering/Driver/DX11/DX11TextureBuffer2D.h>
#include <Rendering/Driver/DX11/DX11VertexBuffer.h>
#include <Rendering/TexelReadback.h>
#include <Rendering/TextureReadback.h>

#include <Utils/DebugUtils/Debug.h>
#include <Utils/Math/Vector2D.h>
#include <Utils/Math/Vector4D.h>
#include <Utils/Platform/Windows/WideStrings.h>

#include <SDL3/SDL.h>

#include <cstring>
#include <iterator>
#include <vector>

#include <d3d9.h>

namespace Rendering
{
	namespace
	{
		D3D_PRIMITIVE_TOPOLOGY ConvertToDX11TopologyType(EPrimitiveTopology topologyType)
		{
			switch (topologyType)
			{
			case EPrimitiveTopology::TriangleList: return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			case EPrimitiveTopology::TriangleStrip: return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
			case EPrimitiveTopology::LineList: return D3D11_PRIMITIVE_TOPOLOGY_LINELIST;

			default: return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
			}
		}

		// Copies a box of a texture's most detailed mip into a staging texture and maps it, waiting for the GPU to
		// finish drawing it, then calls read(mapped) before unmapping. Reads are rare (a click, a capture), so the
		// staging copy is made for each one rather than kept around.
		template <typename ReadFunc>
		void ReadBackRegion(DX11Device& device, ID3D11Texture2D& texture, const D3D11_BOX& box, ReadFunc&& read)
		{
			D3D11_TEXTURE2D_DESC stagingDesc;
			texture.GetDesc(&stagingDesc);
			// D3D only copies depth-stencil textures whole, and wants no box for that
			const bool isWhole = box.left == 0 && box.top == 0 && box.right == stagingDesc.Width && box.bottom == stagingDesc.Height;
			stagingDesc.Width = box.right - box.left;
			stagingDesc.Height = box.bottom - box.top;
			stagingDesc.MipLevels = 1;
			stagingDesc.Usage = D3D11_USAGE_STAGING;
			stagingDesc.BindFlags = 0;
			stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
			stagingDesc.MiscFlags = 0;

			ID3D11Texture2D* staging = nullptr;
			HRESULT hr = device.GetHardwareDevice().CreateTexture2D(&stagingDesc, nullptr, &staging);
			AssertExpr(hr == S_OK);

			ID3D11DeviceContext& deviceContext = device.GetDeviceContext();
			deviceContext.CopySubresourceRegion(staging, 0, 0, 0, 0, &texture, 0, isWhole ? nullptr : &box);

			D3D11_MAPPED_SUBRESOURCE mapped;
			hr = deviceContext.Map(staging, 0, D3D11_MAP_READ, 0, &mapped);
			AssertExpr(hr == S_OK);
			read(mapped);
			deviceContext.Unmap(staging, 0);
			staging->Release();
		}

		// Every resource the device is given came from its own Create functions
		DX11TextureBuffer2D& AsDX11(ITextureBuffer2D& texture)
		{
			return static_cast<DX11TextureBuffer2D&>(texture);
		}
	}

	U32 GetBytesPerTexel(DXGI_FORMAT format)
	{
		switch (format)
		{
		case DXGI_FORMAT_R8_UNORM:
			return 1;
		case DXGI_FORMAT_R16_FLOAT:
		case DXGI_FORMAT_R8G8_UNORM:
			return 2;
		case DXGI_FORMAT_R8G8B8A8_UNORM:
		case DXGI_FORMAT_R16G16_FLOAT:
		case DXGI_FORMAT_R32_UINT:
		case DXGI_FORMAT_R32_FLOAT:
		// Depth-stencil targets that can also be read by shaders (DX11TextureBuffer2D)
		case DXGI_FORMAT_R24G8_TYPELESS:
			return 4;
		case DXGI_FORMAT_R16G16B16A16_FLOAT:
		case DXGI_FORMAT_R32G32_UINT:
			return 8;

		default:
			AssertMsg(false, "A texture format with no known texel size");
			return 0;
		}
	}

	const int kInitialWidth = 1600;
	const int kInitialHeight = 900;

	void DX11Device::Initialize(bool validate)
	{
		// @TODO Need to move a bunch of this code out into RenderEngine
		// and then do these operations via Rendering::Renderer calls. Really the only thing
		// left for the device to do essentially is to create the swapchain and create a device, i think.
		HRESULT hr;

		const D3D11_CREATE_DEVICE_FLAG deviceType = validate ? D3D11_CREATE_DEVICE_DEBUG : D3D11_CREATE_DEVICE_SINGLETHREADED;

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

		// The debug layer only writes its reports to a debugger's output; the info queue holds them for the log. INFO
		// and MESSAGE, the layer's notes on objects made and destroyed, aren't kept.
		if (validate)
		{
			if (SUCCEEDED(mDevice->QueryInterface(__uuidof(ID3D11InfoQueue), reinterpret_cast<void**>(&mInfoQueue))))
			{
				D3D11_MESSAGE_SEVERITY ignoredSeverities[] = { D3D11_MESSAGE_SEVERITY_INFO, D3D11_MESSAGE_SEVERITY_MESSAGE };
				D3D11_INFO_QUEUE_FILTER filter;
				ZeroMemory(&filter, sizeof(filter));
				filter.DenyList.NumSeverities = static_cast<UINT>(std::size(ignoredSeverities));
				filter.DenyList.pSeverityList = ignoredSeverities;
				mInfoQueue->AddStorageFilterEntries(&filter);
			}
			else
			{
				mInfoQueue = nullptr;
				RZE_LOG_WARNING("The D3D11 debug layer's info queue isn't available, so its reports won't be logged.");
			}
		}

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

		// No-cull raster state: as the default but both sides drawn
		{
			D3D11_RASTERIZER_DESC rasterDesc;
			ZeroMemory(&rasterDesc, sizeof(rasterDesc));
			rasterDesc.FillMode = D3D11_FILL_SOLID;
			rasterDesc.CullMode = D3D11_CULL_NONE;
			// As the default, so SV_IsFrontFace means the same
			rasterDesc.FrontCounterClockwise = TRUE;

			mDevice->CreateRasterizerState(&rasterDesc, &mNoCullRasterState);
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

			depthDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
			mDevice->CreateDepthStencilState(&depthDesc, &mDepthReadOnlyLessEqualState);

			depthDesc.DepthEnable = FALSE;
			mDevice->CreateDepthStencilState(&depthDesc, &mDepthOffState);
		}

		// Linear wrap sampler
		{
			D3D11_SAMPLER_DESC samplerDesc;
			ZeroMemory(&samplerDesc, sizeof(samplerDesc));
			samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
			samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
			samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
			samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
			samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
			samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

			mDevice->CreateSamplerState(&samplerDesc, &mLinearWrapSamplerState);
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

		// Linear clamp sampler
		{
			D3D11_SAMPLER_DESC samplerDesc;
			ZeroMemory(&samplerDesc, sizeof(samplerDesc));
			samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
			samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
			samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
			samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
			samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
			samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

			mDevice->CreateSamplerState(&samplerDesc, &mLinearClampSamplerState);
		}

		// Point clamp sampler
		{
			D3D11_SAMPLER_DESC samplerDesc;
			ZeroMemory(&samplerDesc, sizeof(samplerDesc));
			samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
			samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
			samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
			samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
			samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
			samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

			mDevice->CreateSamplerState(&samplerDesc, &mPointClampSamplerState);
		}

		// What creating the swap chain and the states above reported
		ReportDebugMessages();
	}

	void DX11Device::ReportDebugMessages()
	{
		if (mInfoQueue == nullptr)
		{
			return;
		}

		// Each message is a D3D11_MESSAGE followed by its text, so it's read into a buffer of the size the queue gives
		std::vector<Byte> buffer;
		const UINT64 count = mInfoQueue->GetNumStoredMessages();
		for (UINT64 index = 0; index < count; ++index)
		{
			SIZE_T length = 0;
			if (FAILED(mInfoQueue->GetMessage(index, nullptr, &length)) || length == 0)
			{
				continue;
			}
			buffer.resize(length);
			D3D11_MESSAGE* message = reinterpret_cast<D3D11_MESSAGE*>(buffer.data());
			if (FAILED(mInfoQueue->GetMessage(index, message, &length)))
			{
				continue;
			}

			switch (message->Severity)
			{
			case D3D11_MESSAGE_SEVERITY_CORRUPTION:
			case D3D11_MESSAGE_SEVERITY_ERROR:
				RZE_LOG_ERROR("D3D11 (" << static_cast<int>(message->ID) << "): " << message->pDescription);
				break;
			case D3D11_MESSAGE_SEVERITY_WARNING:
				RZE_LOG_WARNING("D3D11 (" << static_cast<int>(message->ID) << "): " << message->pDescription);
				break;
			default:
				// INFO and MESSAGE aren't stored (Initialize)
				break;
			}
		}
		// The queue can't hand messages over one by one, only be cleared, so one another thread stores between the count
		// and here is lost. Not clearing would fill it: it holds 1024 by default, and a warning can come every frame.
		mInfoQueue->ClearStoredMessages();
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
		// The SDL window's HWND, which the swap chain presents to
		mWindowHandle = SDL_GetPointerProperty(SDL_GetWindowProperties(static_cast<SDL_Window*>(windowHandle)), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
	}

	void DX11Device::Shutdown()
	{
		// What's come in since the last present, then the queue goes with the device
		ReportDebugMessages();
		if (mInfoQueue != nullptr)
		{
			mInfoQueue->Release();
			mInfoQueue = nullptr;
		}

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
		mNoCullRasterState->Release();
		mAlphaBlendState->Release();
		mPremultipliedBlendState->Release();
		mAdditiveBlendState->Release();
		mMultiplyBlendState->Release();
		mDepthReadOnlyState->Release();
		mDepthReadOnlyLessEqualState->Release();
		mDepthOffState->Release();
		mLinearWrapSamplerState->Release();
		mShadowSamplerState->Release();
		mLinearClampSamplerState->Release();
		mPointClampSamplerState->Release();
	}

	void DX11Device::SetSyncInterval(U32 interval)
	{
		mSyncInterval = interval;
	}

	bool DX11Device::WaitForPresentSlot(U32 timeoutMS)
	{
		if (mFrameLatencyWaitableObject == nullptr)
		{
			return true;
		}

		return WaitForSingleObjectEx(mFrameLatencyWaitableObject, timeoutMS, TRUE) == WAIT_OBJECT_0;
	}

	std::shared_ptr<IVertexBuffer> DX11Device::CreateVertexBuffer()
	{
		std::shared_ptr<DX11VertexBuffer> buffer = std::make_shared<DX11VertexBuffer>();
		buffer->SetDevice(this);
		return buffer;
	}

	std::shared_ptr<IIndexBuffer> DX11Device::CreateIndexBuffer()
	{
		std::shared_ptr<DX11IndexBuffer> buffer = std::make_shared<DX11IndexBuffer>();
		buffer->SetDevice(this);
		return buffer;
	}

	std::shared_ptr<IConstantBuffer> DX11Device::CreateConstantBuffer(U32 alignment)
	{
		std::shared_ptr<DX11ConstantBuffer> buffer = std::make_shared<DX11ConstantBuffer>(alignment);
		buffer->SetDevice(this);
		return buffer;
	}

	std::shared_ptr<ITextureBuffer2D> DX11Device::CreateTextureBuffer2D()
	{
		std::shared_ptr<DX11TextureBuffer2D> texture = std::make_shared<DX11TextureBuffer2D>();
		texture->SetDevice(this);
		return texture;
	}

	std::shared_ptr<IVertexShader> DX11Device::CreateVertexShader()
	{
		std::shared_ptr<DX11VertexShader> shader = std::make_shared<DX11VertexShader>();
		shader->SetDevice(this);
		return shader;
	}

	std::shared_ptr<IPixelShader> DX11Device::CreatePixelShader()
	{
		std::shared_ptr<DX11PixelShader> shader = std::make_shared<DX11PixelShader>();
		shader->SetDevice(this);
		return shader;
	}

	std::shared_ptr<IComputeShader> DX11Device::CreateComputeShader()
	{
		std::shared_ptr<DX11ComputeShader> shader = std::make_shared<DX11ComputeShader>();
		shader->SetDevice(this);
		return shader;
	}

	void DX11Device::BeginDrawSet()
	{
		// @TODO This is temporary until the API is written for SetRenderTarget() and ClearDepthStencilView()
		// @note past josh wtf api are you talking about
		ID3D11DeviceContext& deviceContext = GetDeviceContext();
		deviceContext.RSSetState(mRasterState);
		deviceContext.PSSetSamplers(0, 1, &mLinearWrapSamplerState);
		deviceContext.CSSetSamplers(0, 1, &mLinearWrapSamplerState);
		deviceContext.PSSetSamplers(1, 1, &mShadowSamplerState);
		deviceContext.CSSetSamplers(1, 1, &mShadowSamplerState);
		deviceContext.PSSetSamplers(2, 1, &mLinearClampSamplerState);
		deviceContext.CSSetSamplers(2, 1, &mLinearClampSamplerState);
		deviceContext.PSSetSamplers(3, 1, &mPointClampSamplerState);
		deviceContext.CSSetSamplers(3, 1, &mPointClampSamplerState);
	}

	bool DX11Device::IsProfilerAttached()
	{
		return D3DPERF_GetStatus() != 0;
	}

	void DX11Device::BeginProfilerEvent(const char* name)
	{
		D3DPERF_BeginEvent(0xffffffff, Conversions::StringToWString(name).c_str());
	}

	void DX11Device::EndProfilerEvent()
	{
		D3DPERF_EndEvent();
	}

	void DX11Device::ClearDepthStencilBuffer(ITextureBuffer2D& depthTexture, bool everySlice, U32 slice)
	{
		DX11TextureBuffer2D* texturePtr = &AsDX11(depthTexture);

		ID3D11DepthStencilView& depthView = everySlice ? texturePtr->GetDepthView() : texturePtr->GetDepthSliceView(slice);
		ID3D11DeviceContext& deviceContext = GetDeviceContext();
		deviceContext.ClearDepthStencilView(&depthView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	}

	void DX11Device::ClearRenderTargets()
	{
		ID3D11RenderTargetView* nullrtv = nullptr;
		ID3D11DeviceContext& deviceContext = GetDeviceContext();
		deviceContext.OMSetRenderTargets(1, &nullrtv, nullptr);
	}

	void DX11Device::SetRenderTargetBackBuffer()
	{
		constexpr FLOAT s_backBufferClearColour[4] = { 0.25f, 0.45f, 0.65f, 1.0f };

		ID3D11DeviceContext& deviceContext = GetDeviceContext();
		deviceContext.ClearRenderTargetView(mRenderTargetView, s_backBufferClearColour);
		deviceContext.ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
		deviceContext.OMSetRenderTargets(1, &mRenderTargetView, mDepthStencilView);
	}

	void DX11Device::SetDepthTarget(ITextureBuffer2D& depthTexture, U32 slice)
	{
		DX11TextureBuffer2D* depthTexturePtr = &AsDX11(depthTexture);

		ID3D11DeviceContext& deviceContext = GetDeviceContext();
		deviceContext.OMSetRenderTargets(0, nullptr, &depthTexturePtr->GetDepthSliceView(slice));
	}

	void DX11Device::SetColourTarget(ITextureBuffer2D& colourTarget, ITextureBuffer2D* depthTexture)
	{
		DX11TextureBuffer2D* targetPtr = &AsDX11(colourTarget);

		ID3D11DepthStencilView* dsv = nullptr;
		if (depthTexture != nullptr)
		{
			dsv = &AsDX11(*depthTexture).GetDepthView();
		}

		ID3D11RenderTargetView* rtv = &targetPtr->GetTargetView();
		ID3D11DeviceContext& deviceContext = GetDeviceContext();
		deviceContext.OMSetRenderTargets(1, &rtv, dsv);
	}

	void DX11Device::GenerateMips(ITextureBuffer2D& texture)
	{
		DX11TextureBuffer2D* texturePtr = &AsDX11(texture);

		ID3D11DeviceContext& deviceContext = GetDeviceContext();
		deviceContext.GenerateMips(&texturePtr->GetResourceView());
	}

	void DX11Device::CopyTexture(ITextureBuffer2D& source, ITextureBuffer2D& destination)
	{
		DX11TextureBuffer2D* sourcePtr = &AsDX11(source);
		DX11TextureBuffer2D* destinationPtr = &AsDX11(destination);

		ID3D11DeviceContext& deviceContext = GetDeviceContext();
		deviceContext.CopyResource(destinationPtr->GetHWResource(), sourcePtr->GetHWResource());
	}

	void DX11Device::CopyTextureRegion(ITextureBuffer2D& source, U32 sourceX, U32 sourceY, U32 width, U32 height, ITextureBuffer2D& destination, U32 destinationX, U32 destinationY)
	{
		DX11TextureBuffer2D* sourcePtr = &AsDX11(source);
		DX11TextureBuffer2D* destinationPtr = &AsDX11(destination);

		const D3D11_BOX box = { sourceX, sourceY, 0, sourceX + width, sourceY + height, 1 };
		ID3D11DeviceContext& deviceContext = GetDeviceContext();
		deviceContext.CopySubresourceRegion(destinationPtr->GetHWResource(), 0, destinationX, destinationY, 0, sourcePtr->GetHWResource(), 0, &box);
	}

	void DX11Device::ClearRenderTarget(ITextureBuffer2D& renderTarget, const Vector4D& colour)
	{
		DX11TextureBuffer2D* texturePtr = &AsDX11(renderTarget);

		ID3D11DeviceContext& deviceContext = GetDeviceContext();
		FLOAT rgba[4] = { colour.X(), colour.Y(), colour.Z(), colour.W() };
		deviceContext.ClearRenderTargetView(&texturePtr->GetTargetView(), rgba);
	}

	void DX11Device::SetViewport(const ViewportParams& params)
	{
		D3D11_VIEWPORT viewport;
		viewport.Width = params.Width;
		viewport.Height = params.Height;
		viewport.MinDepth = params.MinDepth;
		viewport.MaxDepth = params.MaxDepth;
		viewport.TopLeftX = params.TopLeftX;
		viewport.TopLeftY = params.TopLeftY;

		ID3D11DeviceContext& deviceContext = GetDeviceContext();
		deviceContext.RSSetViewports(1, &viewport);
	}

	void DX11Device::SetPrimitiveTopology(EPrimitiveTopology topology)
	{
		D3D_PRIMITIVE_TOPOLOGY d3dTopology = ConvertToDX11TopologyType(topology);
		ID3D11DeviceContext& deviceContext = GetDeviceContext();
		deviceContext.IASetPrimitiveTopology(d3dTopology);
	}

	void DX11Device::SetRasterizerState(ERasterizerState state)
	{
		ID3D11DeviceContext& deviceContext = GetDeviceContext();
		deviceContext.RSSetState(GetRasterizerState(state));
	}

	void DX11Device::SetBlendState(EBlendState state)
	{
		ID3D11DeviceContext& deviceContext = GetDeviceContext();
		deviceContext.OMSetBlendState(GetBlendState(state), nullptr, 0xffffffff);
	}

	void DX11Device::SetDepthState(EDepthState state)
	{
		ID3D11DeviceContext& deviceContext = GetDeviceContext();
		deviceContext.OMSetDepthStencilState(GetDepthState(state), 0);
	}

	void DX11Device::UnsetPixelShader()
	{
		ID3D11DeviceContext& deviceContext = GetDeviceContext();
		deviceContext.PSSetShader(nullptr, nullptr, 0);
	}

	void DX11Device::UnsetTextureResource(U32 textureSlot)
	{
		ID3D11ShaderResourceView* nullSRV = nullptr;
		ID3D11DeviceContext& deviceContext = GetDeviceContext();

		deviceContext.PSSetShaderResources(textureSlot, 1, &nullSRV);
	}

	void DX11Device::UnsetTextureResourceCS(U32 textureSlot)
	{
		ID3D11ShaderResourceView* nullSRV = nullptr;
		ID3D11DeviceContext& deviceContext = GetDeviceContext();

		deviceContext.CSSetShaderResources(textureSlot, 1, &nullSRV);
	}

	void DX11Device::SetUnorderedAccess(ITextureBuffer2D& texture, U32 slot, U32 mip)
	{
		DX11TextureBuffer2D* texturePtr = &AsDX11(texture);

		ID3D11UnorderedAccessView* uav = &texturePtr->GetUnorderedAccessView(mip);
		ID3D11DeviceContext& deviceContext = GetDeviceContext();
		deviceContext.CSSetUnorderedAccessViews(slot, 1, &uav, nullptr);
	}

	void DX11Device::UnsetUnorderedAccess(U32 slot)
	{
		ID3D11UnorderedAccessView* nullUAV = nullptr;
		ID3D11DeviceContext& deviceContext = GetDeviceContext();
		deviceContext.CSSetUnorderedAccessViews(slot, 1, &nullUAV, nullptr);
	}

	void DX11Device::Draw(U32 vertexCount)
	{
		ID3D11DeviceContext& deviceContext = GetDeviceContext();
		deviceContext.Draw(vertexCount, 0);
	}

	void DX11Device::DrawIndexed(U32 indexCount)
	{
		ID3D11DeviceContext& deviceContext = GetDeviceContext();
		deviceContext.DrawIndexed(indexCount, 0, 0);
	}

	void DX11Device::Dispatch(U32 groupsX, U32 groupsY, U32 groupsZ)
	{
		ID3D11DeviceContext& deviceContext = GetDeviceContext();
		deviceContext.Dispatch(groupsX, groupsY, groupsZ);
	}

	void DX11Device::ReadTexel(ITextureBuffer2D& texture, U32 x, U32 y, TexelReadback& result)
	{
		DX11TextureBuffer2D* texturePtr = &AsDX11(texture);

		D3D11_TEXTURE2D_DESC desc;
		texturePtr->GetHWResource()->GetDesc(&desc);
		const U32 bytesPerTexel = GetBytesPerTexel(desc.Format);
		AssertExpr(bytesPerTexel <= sizeof(result.Value));

		const D3D11_BOX texel = { x, y, 0, x + 1, y + 1, 1 };
		ReadBackRegion(*this, *texturePtr->GetHWResource(), texel, [&result, bytesPerTexel](const D3D11_MAPPED_SUBRESOURCE& mapped)
			{
				result.Value = 0;
				memcpy(&result.Value, mapped.pData, bytesPerTexel);
			});

		result.IsReady.store(true, std::memory_order_release);
	}

	void DX11Device::ReadTexture(ITextureBuffer2D& texture, TextureReadback& result)
	{
		DX11TextureBuffer2D* texturePtr = &AsDX11(texture);

		D3D11_TEXTURE2D_DESC desc;
		texturePtr->GetHWResource()->GetDesc(&desc);

		result.Width = desc.Width;
		result.Height = desc.Height;
		result.BytesPerTexel = GetBytesPerTexel(desc.Format);

		const size_t rowSize = static_cast<size_t>(result.Width) * result.BytesPerTexel;
		result.Data.resize(rowSize * result.Height);

		const D3D11_BOX whole = { 0, 0, 0, desc.Width, desc.Height, 1 };
		ReadBackRegion(*this, *texturePtr->GetHWResource(), whole, [&result, rowSize](const D3D11_MAPPED_SUBRESOURCE& mapped)
			{
				// Mapped rows can be padded past the texture's width
				const U8* source = static_cast<const U8*>(mapped.pData);
				for (U32 row = 0; row < result.Height; ++row)
				{
					memcpy(&result.Data[row * rowSize], source + static_cast<size_t>(row) * mapped.RowPitch, rowSize);
				}
			});

		result.IsReady.store(true, std::memory_order_release);
	}

	void DX11Device::ReadBackBuffer(TextureReadback& result)
	{
		// Buffer 0 is the one being drawn, until Present (FLIP_DISCARD) hands it to the window
		ID3D11Texture2D* backBuffer = nullptr;
		HRESULT hr = mSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer));
		AssertExpr(hr == S_OK);

		D3D11_TEXTURE2D_DESC desc;
		backBuffer->GetDesc(&desc);
		result.Width = desc.Width;
		result.Height = desc.Height;
		// R8G8B8A8_UNORM, as the swap chain is made
		result.BytesPerTexel = GetBytesPerTexel(desc.Format);

		const size_t rowSize = static_cast<size_t>(result.Width) * result.BytesPerTexel;
		result.Data.resize(rowSize * result.Height);

		const D3D11_BOX whole = { 0, 0, 0, desc.Width, desc.Height, 1 };
		ReadBackRegion(*this, *backBuffer, whole, [&result, rowSize](const D3D11_MAPPED_SUBRESOURCE& mapped)
			{
				const U8* source = static_cast<const U8*>(mapped.pData);
				for (U32 row = 0; row < result.Height; ++row)
				{
					memcpy(&result.Data[row * rowSize], source + static_cast<size_t>(row) * mapped.RowPitch, rowSize);
				}
			});
		backBuffer->Release();

		result.IsReady.store(true, std::memory_order_release);
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
		// Everything the frame's calls reported
		ReportDebugMessages();

		if ((hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) && !mHasReportedDeviceLoss)
		{
			mHasReportedDeviceLoss = true;
			RZE_LOG_ERROR_ARGS("Present failed: the D3D11 device was %s (reason 0x%08X).", hr == DXGI_ERROR_DEVICE_REMOVED ? "removed" : "reset",
				static_cast<unsigned int>(mDevice->GetDeviceRemovedReason()));
		}
	}

	ID3D11RasterizerState* DX11Device::GetRasterizerState(ERasterizerState state) const
	{
		switch (state)
		{
		case ERasterizerState::ShadowCaster:
			return mShadowCasterRasterState;
		case ERasterizerState::NoCull:
			return mNoCullRasterState;
		default:
			return mRasterState;
		}
	}

	ID3D11BlendState* DX11Device::GetBlendState(EBlendState state) const
	{
		switch (state)
		{
		case EBlendState::AlphaBlend:
			return mAlphaBlendState;
		case EBlendState::Premultiplied:
			return mPremultipliedBlendState;
		case EBlendState::Additive:
			return mAdditiveBlendState;
		case EBlendState::Multiply:
			return mMultiplyBlendState;
		default:
			return nullptr;
		}
	}

	ID3D11DepthStencilState* DX11Device::GetDepthState(EDepthState state) const
	{
		switch (state)
		{
		case EDepthState::ReadOnly:
			return mDepthReadOnlyState;
		case EDepthState::ReadOnlyLessEqual:
			return mDepthReadOnlyLessEqualState;
		case EDepthState::Off:
			return mDepthOffState;
		default:
			return nullptr;
		}
	}
}