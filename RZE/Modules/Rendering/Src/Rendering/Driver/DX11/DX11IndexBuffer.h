#pragma once

#include <Rendering/Driver/GFXBuffer.h>

struct ID3D11Buffer;

namespace Rendering
{
	class DX11Device;

	class DX11IndexBuffer : public IGPUBuffer
	{
	public:
		DX11IndexBuffer() = default;
		~DX11IndexBuffer();

	// IIndexBuffer interface
	public:
		virtual void Allocate(void* data, size_t size, U32 count) override;
		virtual void Release() override;

		virtual void SetActive() override;

	public:
		void SetDevice(DX11Device* device);
		ID3D11Buffer& GetHardwareBuffer();

		U32 GetIndexCount();

	private:
		U32 mIndexCount;

	private:
		DX11Device* m_device;
		ID3D11Buffer* m_hwBuffer;
	};

}