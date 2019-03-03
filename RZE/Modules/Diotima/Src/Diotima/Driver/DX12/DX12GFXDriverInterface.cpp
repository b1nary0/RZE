#include <Diotima/Driver/DX12/DX12GFXDriverInterface.h>

#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_4.h>

#include <Diotima/Driver/DX12/DX12GFXDevice.h>

#include <Utils/DebugUtils/Debug.h>

namespace Diotima
{

	void DX12GFXDriverInterface::Initialize()
	{
#if defined(_DEBUG)
 		ComPtr<ID3D12Debug> debugController;
 		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
 		{
 			debugController->EnableDebugLayer();
 		}
#endif

		mDevice = std::make_unique<DX12GFXDevice>();
		mDevice->SetWindow(mWindowHandle);
		mDevice->Initialize();
	}

	void DX12GFXDriverInterface::SetWindow(void* windowHandle)
	{
		AssertNotNull(windowHandle);
		mWindowHandle = windowHandle;
	}

	void DX12GFXDriverInterface::Shutdown()
	{
		mDevice->Shutdown();
	}

	void DX12GFXDriverInterface::Present()
	{
		mDevice->Present();
	}

	U32 DX12GFXDriverInterface::CreateVertexBuffer(void* data, U32 numElements)
	{
		return mDevice->CreateVertexBuffer(data, numElements);
	}

	U32 DX12GFXDriverInterface::CreateIndexBuffer(void* data, U32 numElements)
	{
		return mDevice->CreateIndexBuffer(data, numElements);
	}

	U32 DX12GFXDriverInterface::CreateTextureBuffer2D(void* data, U32 width, U32 height)
	{
		return mDevice->CreateTextureBuffer2D(data, width, height);
	}

}