#include <Rendering/Driver/DX11/DX11IndexBuffer.h>

#include <Rendering/Driver/DX11/DX11Device.h>

#include <Rendering/Driver/DX11/DX11.h>

#include <Utils/DebugUtils/Debug.h>

namespace Rendering
{

	void DX11IndexBuffer::Allocate(void* data, size_t size, U32 count)
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
		hr = m_device->GetHardwareDevice().CreateBuffer(&indexBufferDesc, &indexBufferData, &m_hwBuffer);
		AssertExpr(hr == S_OK);
	}

	void DX11IndexBuffer::Release()
	{
		AssertNotNull(m_hwBuffer);
		m_hwBuffer->Release();
	}

	void DX11IndexBuffer::SetActive()
	{
		ID3D11DeviceContext& deviceContext = m_device->GetDeviceContext();
		deviceContext.IASetIndexBuffer(m_hwBuffer, DXGI_FORMAT_R32_UINT, 0);
	}

	void DX11IndexBuffer::SetDevice(DX11Device* device)
	{
		AssertNotNull(device);
		m_device = device;
	}

	ID3D11Buffer& DX11IndexBuffer::GetHardwareBuffer()
	{
		AssertNotNull(m_hwBuffer);
		return *m_hwBuffer;
	}

	U32 DX11IndexBuffer::GetIndexCount()
	{
		return mIndexCount;
	}

}