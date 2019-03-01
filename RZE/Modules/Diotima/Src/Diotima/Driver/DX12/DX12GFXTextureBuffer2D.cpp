#include <Diotima/Driver/DX12/DX12GFXTextureBuffer2D.h>

#include <Diotima/Driver/DX12/DX12GFXDevice.h>

#include <Utils/DebugUtils/Debug.h>

namespace Diotima
{
	void DX12GFXTextureBuffer2D::Allocate(void* data, U32 width, U32 height)
	{
		
	}

	void DX12GFXTextureBuffer2D::SetDevice(DX12GFXDevice* device)
	{
		AssertNotNull(device);
		mDevice = device;
	}

}