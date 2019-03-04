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
		virtual ~DX12GFXConstantBuffer();

	public:
		virtual void Allocate(void* data, U32 numElements) override;

	public:
		void SetDevice(DX12GFXDevice* device);

		ID3D12DescriptorHeap* GetDescriptorHeap();
		ID3D12Resource* GetResource();

		U32 GetNumElements() const;

		// #TODO(Josh::Yeah so this is garbo -- this should be done with some internal structure per object that describes the buffers/views/etc - future work)
		void SetData(void* data, U32 size, U32 objectIndex);

	private:
		ComPtr<ID3D12Resource> mUploadBuffer;
		ComPtr<ID3D12DescriptorHeap> mDescriptorHeap;

		DX12GFXDevice* mDevice;

		U32 mNumElements;

		void* mResource;
	};
}