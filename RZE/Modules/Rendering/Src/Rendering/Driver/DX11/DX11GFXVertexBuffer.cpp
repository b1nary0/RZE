#include <Rendering/Driver/DX11/DX11GFXVertexBuffer.h>

#include <Rendering/Driver/DX11/DX11GFXDevice.h>

#include <Rendering/Driver/DX11/DX11.h>

#include <Utils/DebugUtils/Debug.h>

namespace Rendering
{

	void DX11GFXVertexBuffer::Allocate(void* data, size_t size, U32 count)
	{
		D3D11_BUFFER_DESC vertexBufferDesc;
		ZeroMemory(&vertexBufferDesc, sizeof(vertexBufferDesc));

		vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
		vertexBufferDesc.ByteWidth = size * count;
		vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vertexBufferDesc.CPUAccessFlags = 0;
		vertexBufferDesc.MiscFlags = 0;

		D3D11_SUBRESOURCE_DATA vertexBufferData;
		ZeroMemory(&vertexBufferData, sizeof(vertexBufferData));

		HRESULT hr;
		vertexBufferData.pSysMem = data;
		hr = mDevice->GetHardwareDevice().CreateBuffer(&vertexBufferDesc, &vertexBufferData, &mBuf);
		AssertExpr(hr == S_OK);
	}

	void DX11GFXVertexBuffer::Release()
	{
		AssertNotNull(mBuf);
		mBuf->Release();
	}

	ID3D11Buffer& DX11GFXVertexBuffer::GetHardwareBuffer()
	{
		AssertNotNull(mBuf);
		return *mBuf;
	}

	void DX11GFXVertexBuffer::SetDevice(DX11GFXDevice* device)
	{
		mDevice = device;
	}

}