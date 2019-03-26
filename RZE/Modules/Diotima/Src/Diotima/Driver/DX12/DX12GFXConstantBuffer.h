#pragma once

#include <Diotima/Driver/DX12/DX12.h>

#include <Utils/PrimitiveDefs.h>

namespace Diotima
{
	class DX12GFXDevice;

	class DX12GFXConstantBuffer
	{
	public:
		DX12GFXConstantBuffer() = default;
		virtual ~DX12GFXConstantBuffer();

	public:
		void Allocate(size_t memberSize, U32 maxMembers);

	public:
		void SetDevice(DX12GFXDevice* device);

		ID3D12DescriptorHeap* GetDescriptorHeap();
		ID3D12Resource* GetResource();

		// #TODO(Josh::Yeah so this is garbo -- this should be done with some internal structure per object that describes the buffers/views/etc - future work)
		void SetData(const void* data, U32 size, U32 objectIndex);

	private:
		ComPtr<ID3D12Resource> mUploadBuffer;
		ComPtr<ID3D12DescriptorHeap> mDescriptorHeap;

		DX12GFXDevice* mDevice;

		void* mResource;
	};
}