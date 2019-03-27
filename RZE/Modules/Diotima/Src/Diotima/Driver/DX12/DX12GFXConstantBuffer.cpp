#pragma once
#include <Diotima/Driver/DX12/DX12GFXConstantBuffer.h>

#include <Diotima/Driver/DX12/DX12GFXDevice.h>

#include <Utils/MemoryUtils.h>
#include <Utils/DebugUtils/Debug.h>

namespace Diotima
{

	DX12GFXConstantBuffer::DX12GFXConstantBuffer()
		: mNumAllocations(0)
	{
	}

	DX12GFXConstantBuffer::~DX12GFXConstantBuffer()
	{
	}

	void DX12GFXConstantBuffer::Allocate(size_t memberSize, U32 maxMembers)
	{
		const U32 alignedSize = MemoryUtils::AlignSize(memberSize, 255);
		const U32 bufferSize = alignedSize * maxMembers;

		mMemberSize = memberSize;
		mBufferSize = bufferSize;

		mDevice->GetDevice()->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), 
			D3D12_HEAP_FLAG_NONE, 
			&CD3DX12_RESOURCE_DESC::Buffer(bufferSize), 
			D3D12_RESOURCE_STATE_GENERIC_READ, 
			nullptr, 
			IID_PPV_ARGS(&mUploadBuffer));

		CD3DX12_RANGE readRange(0, 0);
		mUploadBuffer->Map(0, &readRange, reinterpret_cast<void**>(&mResourceStart));

		mCurrAddr = mResourceStart;
		mResourceEnd = mResourceStart + mBufferSize;
	}

	CBAllocationData DX12GFXConstantBuffer::AllocateMember(const void* data)
	{
		U32 alignedSize = MemoryUtils::AlignSize(mMemberSize, 255);
		AssertExpr(mCurrAddr + alignedSize < mResourceEnd);

		memcpy(mCurrAddr, data, mMemberSize);
		mCurrAddr = mCurrAddr + alignedSize;
		++mNumAllocations;

		CBAllocationData allocData;
		allocData.GPUBaseAddr = mUploadBuffer->GetGPUVirtualAddress() + ((mNumAllocations - 1) * alignedSize);
		allocData.SizeInBytes = alignedSize;
		return allocData;
	}

	void DX12GFXConstantBuffer::Reset()
	{
		mNumAllocations = 0;
		mCurrAddr = mResourceStart;
	}

	void DX12GFXConstantBuffer::SetDevice(DX12GFXDevice* device)
	{
		AssertNotNull(device);
		mDevice = device;
	}

	ID3D12Resource* DX12GFXConstantBuffer::GetResource()
	{
		return mUploadBuffer.Get();
	}

}