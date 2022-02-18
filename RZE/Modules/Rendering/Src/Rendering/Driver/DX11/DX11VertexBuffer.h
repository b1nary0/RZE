#pragma once

#include <Rendering/Driver/GFXBuffer.h>

struct ID3D11Buffer;

namespace Rendering
{
	class DX11Device;

	class DX11VertexBuffer : public IVertexBuffer
	{
	public:
		DX11VertexBuffer() = default;
		~DX11VertexBuffer();

		// IVertexBuffer interface
	public:
		void Allocate(const void* data, size_t size, U32 count, U32 stride) override;
		void Release() override;

		void SetActive(U32 bufferSlot) override;

	public:
		void SetDevice(DX11Device* device);

	private:
		U32 m_stride;
		U32 m_offset;

		DX11Device* m_device;
		ID3D11Buffer* m_buffer;
	};

}