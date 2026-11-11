#pragma once

#include <Rendering/Driver/GFXBuffer.h>

namespace Rendering
{
	class GLDevice;

	class GLVertexBuffer : public IVertexBuffer
	{
	public:
		explicit GLVertexBuffer(GLDevice& device);

	// IVertexBuffer interface
	public:
		void SetDynamic(bool isDynamic) override;
		void Allocate(const void* data, size_t size, U32 count, U32 stride) override;
		void Release() override;
		void UpdateData(const void* data, size_t bytes) override;
		void SetActive(U32 bufferSlot) override;

	public:
		~GLVertexBuffer() override;

		U32 GetBuffer() const { return m_buffer; }
		U32 GetStride() const { return m_stride; }

	private:
		GLDevice& m_device;
		U32 m_buffer = 0;
		U32 m_stride = 0;
		size_t m_capacityBytes = 0;
		bool m_isDynamic = false;
	};
}
