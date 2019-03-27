#pragma once

#include <Diotima/Driver/DX12/DX12.h>

#include <Utils/PrimitiveDefs.h>

namespace Diotima
{
	class DX12GFXDevice;

	// #TODO(Josh::If this ends up working well move this into a more generic header)
	struct CBAllocationData
	{
		D3D12_GPU_VIRTUAL_ADDRESS GPUBaseAddr;
		size_t SizeInBytes;
	};

	class DX12GFXConstantBuffer
	{
	public:
		DX12GFXConstantBuffer();
		virtual ~DX12GFXConstantBuffer();

	public:
		void Allocate(size_t memberSize, U32 maxMembers);
		CBAllocationData AllocateMember(const void* data);

		void Reset();

	public:
		void SetDevice(DX12GFXDevice* device);

		ID3D12Resource* GetResource();

	private:
		DX12GFXDevice* mDevice;
		
		ComPtr<ID3D12Resource> mUploadBuffer;

		size_t mMemberSize;
		U32 mBufferSize;
		U32 mNumAllocations;

		U8* mResourceStart;
		U8* mResourceEnd;
		U8* mCurrAddr;
	};
}