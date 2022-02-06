#pragma once

#include <Rendering/Driver/GFXBuffer.h>

struct ID3D11Buffer;

namespace Rendering
{
	class DX11Device;

	class DX11ConstantBuffer : public IConstantBuffer
	{
	public:
		DX11ConstantBuffer() = default;
		~DX11ConstantBuffer() = default;

	// IGFXConstantBuffer interface
	public:
		void Allocate(const void* data, size_t size, U32 count) override;
		void Release() override;


		void SetActiveVS(U32 bufferSlot) override;
		void SetActivePS(U32 bufferSlot) override;

	public:
		void SetDevice(DX11Device* device);
		ID3D11Buffer& GetHardwareBuffer();

		void UpdateSubresources(const void* data);

	private:
		DX11Device* m_device = nullptr;
		ID3D11Buffer* m_buffer = nullptr;
	};
}