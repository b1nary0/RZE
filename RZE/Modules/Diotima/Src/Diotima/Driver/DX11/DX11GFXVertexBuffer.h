#pragma once

#include <Diotima/Driver/GFXBuffer.h>

struct ID3D11Buffer;

namespace Diotima
{
	class DX11GFXDevice;

	class DX11GFXVertexBuffer : public IGFXVertexBuffer
	{
	public:
		DX11GFXVertexBuffer() = default;
		~DX11GFXVertexBuffer() = default;

		// IGFXVertexBuffer interface
	public:
		virtual void Allocate(void* data, size_t size, U32 count) override;
		virtual void Release() override;

	public:
		ID3D11Buffer& GetHardwareBuffer();
		void SetDevice(DX11GFXDevice* device);

	private:
		U32 mStride;
		U32 mOffset;

		DX11GFXDevice* mDevice;
		ID3D11Buffer* mBuf;
	};

}