#pragma once

#include <Diotima/Driver/GFXBuffer.h>

#include <Diotima/Driver/DX12/DX12.h>

namespace Diotima
{
	class DX12GFXDevice;

	class DX12GFXConstantBuffer final : public IGFXConstantBuffer
	{
	public:
		DX12GFXConstantBuffer() = default;
		virtual ~DX12GFXConstantBuffer() = default;

	public:
		virtual void Allocate(void* data, U32 numElements) override;

	public:
		void SetDevice(DX12GFXDevice* device);

		U32 GetNumElements() const;

		void SetData(void* data);

	private:
		ComPtr<ID3D12Resource> mUploadBuffer;
		ComPtr<ID3D12DescriptorHeap> mDescriptorHeap;

		DX12GFXDevice* mDevice;

		U32 mNumElements;

		void* mResource;
	};
}