#pragma once

#include <Diotima/Driver/GFXBuffer.h>

struct ID3D11Buffer;

namespace Diotima
{
	class DX11GFXDevice;

	class DX11GFXIndexBuffer : public IGFXIndexBuffer
	{
	public:
		DX11GFXIndexBuffer() = default;
		~DX11GFXIndexBuffer() = default;

	// IGFXIndexBuffer interface
	public:
		virtual void Allocate(void* data, size_t size, U32 count) override;

	public:
		void SetDevice(DX11GFXDevice* device);
		ID3D11Buffer& GetHardwareBuffer();

	private:
		DX11GFXDevice* mDevice;
		ID3D11Buffer* mBuf;
	};

}