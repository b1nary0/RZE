#pragma once

#include <Rendering/Driver/GFXBuffer.h>

namespace Rendering
{
	class GLDevice;

	class GLConstantBuffer : public IConstantBuffer
	{
	public:
		GLConstantBuffer(GLDevice& device, U32 alignment);

	// IConstantBuffer interface
	public:
		void Allocate(const void* data, size_t size, U32 count) override;
		void Release() override;
		void UpdateData(const void* data) override;
		void SetActiveVS(U32 bufferSlot) override;
		void SetActivePS(U32 bufferSlot) override;
		void SetActiveCS(U32 bufferSlot) override;
		U32 GetAlignment() override;

	private:
		[[maybe_unused]] GLDevice& m_device;
		U32 m_alignment = 0;
	};
}
