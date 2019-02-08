#include <Diotima/Driver/DX12/DX12GFXDriverInterface.h>

#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_4.h>

#include <Diotima/Driver/DX12/DX12GFXDevice.h>

namespace Diotima
{

	void DX12GFXDriverInterface::Initialize()
	{
#if defined(_DEBUG)
// 		ComPtr<ID3D12Debug> debugController;
// 		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
// 		{
// 			debugController->EnableDebugLayer();
// 		}
#endif
	}

}