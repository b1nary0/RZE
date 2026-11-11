#pragma once

#include <Rendering/Driver/GFXBuffer.h>

namespace Rendering
{
	class GLDevice;

	class GLIndexBuffer : public IIndexBuffer
	{
	public:
		explicit GLIndexBuffer(GLDevice& device);

	// IIndexBuffer interface
	public:
		void Allocate(const void* data, size_t size, U32 count) override;
		void Release() override;
		void SetActive() override;
		U32 GetIndexCount() override;

	public:
		~GLIndexBuffer() override;

	private:
		GLDevice& m_device;
		U32 m_buffer = 0;
		U32 m_indexCount = 0;
	};
}
