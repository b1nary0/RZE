#include <Diotima/Driver/DX11/DX11GFXIndexBuffer.h>

#include <Diotima/Driver/DX11/DX11GFXDevice.h>

#include <Diotima/Driver/DX11/DX11.h>

#include <Utils/DebugUtils/Debug.h>

namespace Diotima
{

	void DX11GFXIndexBuffer::Allocate(void* data, size_t size, U32 count)
	{
		mIndexCount = count;

		D3D11_BUFFER_DESC indexBufferDesc;
		ZeroMemory(&indexBufferDesc, sizeof(indexBufferDesc));

		indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
		indexBufferDesc.ByteWidth = size * count;
		indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		indexBufferDesc.CPUAccessFlags = 0;
		indexBufferDesc.MiscFlags = 0;

		D3D11_SUBRESOURCE_DATA indexBufferData;
		ZeroMemory(&indexBufferData, sizeof(indexBufferData));

		indexBufferData.pSysMem = data;

		HRESULT hr;
		hr = mDevice->GetHardwareDevice().CreateBuffer(&indexBufferDesc, &indexBufferData, &mBuf);
		AssertExpr(hr == S_OK);
	}

	void DX11GFXIndexBuffer::Release()
	{
		AssertNotNull(mBuf);
		mBuf->Release();
	}

	void DX11GFXIndexBuffer::SetDevice(DX11GFXDevice* device)
	{
		AssertNotNull(device);
		mDevice = device;
	}

	ID3D11Buffer& DX11GFXIndexBuffer::GetHardwareBuffer()
	{
		AssertNotNull(mBuf);
		return *mBuf;
	}

	U32 DX11GFXIndexBuffer::GetIndexCount()
	{
		return mIndexCount;
	}

}