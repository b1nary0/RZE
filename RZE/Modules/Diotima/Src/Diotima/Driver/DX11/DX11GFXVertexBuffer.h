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
		virtual void Allocate(void* data, U32 numElements) override;

	public:
		void SetDevice(DX11GFXDevice* device);

	private:
		DX11GFXDevice* mDevice;
		ID3D11Buffer* mBuf;
	};

}