#include <Diotima/Driver/DX11/DX11GFXVertexBuffer.h>

#include <Diotima/Driver/DX11/DX11GFXDevice.h>

#include <Diotima/Driver/DX11/DX11.h>

namespace Diotima
{

	void DX11GFXVertexBuffer::Allocate(void* data, U32 numElements)
	{
		D3D11_BUFFER_DESC vertexBufferDesc;
		ZeroMemory(&vertexBufferDesc, sizeof(vertexBufferDesc));

		vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
		vertexBufferDesc.ByteWidth = (sizeof(float) * 3) * numElements;
		vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vertexBufferDesc.CPUAccessFlags = 0;
		vertexBufferDesc.MiscFlags = 0;

		D3D11_SUBRESOURCE_DATA vertexBufferData;
		ZeroMemory(&vertexBufferData, sizeof(vertexBufferData));

		HRESULT hr;
		vertexBufferData.pSysMem = data;
		hr = mDevice->GetHardwareDevice().CreateBuffer(&vertexBufferDesc, &vertexBufferData, &mBuf);
	}

	void DX11GFXVertexBuffer::SetDevice(DX11GFXDevice* device)
	{
		mDevice = device;
	}

}