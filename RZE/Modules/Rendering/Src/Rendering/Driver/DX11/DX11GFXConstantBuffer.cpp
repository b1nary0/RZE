#include <Rendering/Driver/DX11/DX11GFXConstantBuffer.h>

#include <Rendering/Driver/DX11/DX11Device.h>

#include <Rendering/Driver/DX11/DX11.h>

#include <Utils/DebugUtils/Debug.h>

namespace Rendering
{

	void DX11GFXConstantBuffer::Allocate(size_t size, U32 count)
	{
		D3D11_BUFFER_DESC bufferDesc;
		ZeroMemory(&bufferDesc, sizeof(bufferDesc));

		bufferDesc.Usage = D3D11_USAGE_DEFAULT;
		bufferDesc.ByteWidth = size * count;
		bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bufferDesc.CPUAccessFlags = 0;
		bufferDesc.MiscFlags = 0;

		HRESULT hr = mDevice->GetHardwareDevice().CreateBuffer(&bufferDesc, NULL, &mBuf);
		AssertExpr(hr == S_OK);
	}

	void DX11GFXConstantBuffer::Release()
	{
		AssertNotNull(mBuf);
		mBuf->Release();
	}

	void DX11GFXConstantBuffer::SetDevice(DX11Device* device)
	{
		AssertNotNull(device);
		mDevice = device;
	}

	ID3D11Buffer& DX11GFXConstantBuffer::GetHardwareBuffer()
	{
		AssertNotNull(mBuf);
		return *mBuf;
	}

	void DX11GFXConstantBuffer::UpdateSubresources(const void* data)
	{
		mDevice->GetDeviceContext().UpdateSubresource(mBuf, 0, NULL, data, 0, 0);
	}

}