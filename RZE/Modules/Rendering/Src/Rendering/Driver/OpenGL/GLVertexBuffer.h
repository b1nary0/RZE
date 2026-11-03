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

	private:
		[[maybe_unused]] GLDevice& m_device;
		bool m_isDynamic = false;
	};
}
