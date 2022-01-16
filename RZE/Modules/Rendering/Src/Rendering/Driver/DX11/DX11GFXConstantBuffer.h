#pragma once

#include <Rendering/Driver/GFXBuffer.h>

struct ID3D11Buffer;

namespace Rendering
{
	class DX11GFXDevice;

	class DX11GFXConstantBuffer : public IGFXConstantBuffer
	{
	public:
		DX11GFXConstantBuffer() = default;
		~DX11GFXConstantBuffer() = default;

	// IGFXConstantBuffer interface
	public:
		virtual void Allocate(size_t size, U32 count) override;
		virtual void Release() override;

	public:
		void SetDevice(DX11GFXDevice* device);
		ID3D11Buffer& GetHardwareBuffer();

		void UpdateSubresources(const void* data);

	private:
		DX11GFXDevice* mDevice;
		ID3D11Buffer* mBuf;
	};
}