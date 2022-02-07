#pragma once

#include <Rendering/Driver/GFXBuffer.h>

struct ID3D11Buffer;

namespace Rendering
{
	class DX11Device;

	class DX11IndexBuffer : public IIndexBuffer
	{
	public:
		DX11IndexBuffer() = default;
		~DX11IndexBuffer();

	// IIndexBuffer interface
	public:
		void Allocate(const void* data, size_t size, U32 count) override;
		void Release() override;

		void SetActive() override;

		U32 GetIndexCount() override;

	public:
		void SetDevice(DX11Device* device);
		ID3D11Buffer& GetHardwareBuffer();


	private:
		U32 mIndexCount;

	private:
		DX11Device* m_device;
		ID3D11Buffer* m_hwBuffer;
	};

}