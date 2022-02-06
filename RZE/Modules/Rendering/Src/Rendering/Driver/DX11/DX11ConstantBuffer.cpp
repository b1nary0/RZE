#include <Rendering/Driver/DX11/DX11ConstantBuffer.h>

#include <Rendering/Driver/DX11/DX11Device.h>

#include <Rendering/Driver/DX11/DX11.h>

#include <Utils/DebugUtils/Debug.h>

namespace Rendering
{

	void DX11ConstantBuffer::Allocate(const void* data, size_t size, U32 count)
	{
		D3D11_BUFFER_DESC bufferDesc;
		ZeroMemory(&bufferDesc, sizeof(bufferDesc));

		bufferDesc.Usage = D3D11_USAGE_DEFAULT;
		bufferDesc.ByteWidth = size * count;
		bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bufferDesc.CPUAccessFlags = 0;
		bufferDesc.MiscFlags = 0;

		HRESULT hr = m_device->GetHardwareDevice().CreateBuffer(&bufferDesc, nullptr, &m_buffer);
		AssertExpr(hr == S_OK);
	}

	void DX11ConstantBuffer::Release()
	{
		AssertNotNull(m_buffer);
		m_buffer->Release();
	}

	void DX11ConstantBuffer::SetActiveVS(U32 bufferSlot)
	{
		ID3D11DeviceContext& deviceContext = m_device->GetDeviceContext();
		deviceContext.VSSetConstantBuffers(bufferSlot, 1, &m_buffer);
	}

	void DX11ConstantBuffer::SetActivePS(U32 bufferSlot)
	{
		ID3D11DeviceContext& deviceContext = m_device->GetDeviceContext();
		deviceContext.PSSetConstantBuffers(bufferSlot, 1, &m_buffer);
	}

	void DX11ConstantBuffer::SetDevice(DX11Device* device)
	{
		AssertNotNull(device);
		m_device = device;
	}

	ID3D11Buffer& DX11ConstantBuffer::GetHardwareBuffer()
	{
		AssertNotNull(m_buffer);
		return *m_buffer;
	}

	void DX11ConstantBuffer::UpdateSubresources(const void* data)
	{
		m_device->GetDeviceContext().UpdateSubresource(m_buffer, 0, nullptr, data, 0, 0);
	}

}