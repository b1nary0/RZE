#pragma once

#include <Diotima/Driver/DX12/DX12.h>
#include <Diotima/Driver/DX12/DX12AllocationData.h>

#include <Utils/PrimitiveDefs.h>

namespace Diotima
{
	class DX12GFXDevice;

	class DX12GFXConstantBuffer
	{
	public:
		DX12GFXConstantBuffer();
		virtual ~DX12GFXConstantBuffer();

	public:
		void Allocate(size_t memberSize, U32 maxMembers);
		CBAllocationData AllocateMember(const void* data);

		CBAllocationData GetBufferStart();

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