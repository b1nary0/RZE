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
		// Must be called before Allocate(). Dynamic buffers are created empty and written via UpdateData().
		void SetDynamic(bool isDynamic) { m_isDynamic = isDynamic; }

		// Overwrites the buffer contents (WRITE_DISCARD). Dynamic buffers only.
		void UpdateData(const void* data, size_t bytes);

	private:
		U32 m_stride;
		U32 m_offset;

		bool m_isDynamic = false;
		size_t m_capacityBytes = 0;

		DX11Device* m_device;
		ID3D11Buffer* m_buffer;
	};

}