#pragma once

#include <Rendering/Driver/GFXBuffer.h>

struct ID3D11Buffer;

namespace Rendering
{
	class DX11Device;

	class DX11ConstantBuffer : public IConstantBuffer
	{
	public:
		DX11ConstantBuffer() = delete;
		DX11ConstantBuffer(U32 alignment)
			: m_alignment(alignment) {}

		~DX11ConstantBuffer() = default;

	// IGFXConstantBuffer interface
	public:
		void Allocate(const void* data, size_t size, U32 count) override;
		void Release() override;

		void SetActiveVS(U32 bufferSlot) override;
		void SetActivePS(U32 bufferSlot) override;

		U32 GetAlignment() override;

	public:
		void SetDevice(DX11Device* device);
		ID3D11Buffer& GetHardwareBuffer();

		void UpdateSubresources(const void* data);

	private:
		U32 m_alignment = 0;

		DX11Device* m_device = nullptr;
		ID3D11Buffer* m_buffer = nullptr;
	};
}