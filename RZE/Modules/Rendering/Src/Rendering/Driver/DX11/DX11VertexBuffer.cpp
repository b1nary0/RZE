#include <Rendering/Driver/DX11/DX11VertexBuffer.h>

#include <Rendering/Driver/DX11/DX11Device.h>
#include <Rendering/Driver/DX11/DX11.h>

#include <Utils/Math/Vector2D.h>
#include <Utils/Math/Vector3D.h>

#include <Utils/DebugUtils/Debug.h>

namespace Rendering
{
	DX11VertexBuffer::~DX11VertexBuffer()
	{
		Release();
	}

	void DX11VertexBuffer::Allocate(const void* data, size_t size, U32 count, U32 stride)
	{
		m_stride = stride;
		m_offset = 0;

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
		hr = m_device->GetHardwareDevice().CreateBuffer(&vertexBufferDesc, &vertexBufferData, &m_buffer);
		AssertExpr(hr == S_OK);
	}

	void DX11VertexBuffer::Release()
	{
		AssertNotNull(m_buffer);
		m_buffer->Release();
	}

	void DX11VertexBuffer::SetActive(U32 bufferSlot)
	{
		ID3D11DeviceContext& deviceContext = m_device->GetDeviceContext();
		deviceContext.IASetVertexBuffers(bufferSlot, 1, &m_buffer, &m_stride, &m_offset);
	}
	
	void DX11VertexBuffer::SetDevice(DX11Device* device)
	{
		m_device = device;
	}

}