#pragma once
#include <Diotima/Driver/DX12/DX12GFXConstantBuffer.h>

#include <Diotima/Driver/DX12/DX12GFXDevice.h>

#include <Utils/MemoryUtils.h>
#include <Utils/DebugUtils/Debug.h>

namespace Diotima
{

	DX12GFXConstantBuffer::~DX12GFXConstantBuffer()
	{
	}

	void DX12GFXConstantBuffer::Allocate(void* data, U32 numElements)
	{
		mNumElements = numElements;

		mDevice->GetDevice()->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), 
			D3D12_HEAP_FLAG_NONE, 
			&CD3DX12_RESOURCE_DESC::Buffer(1024 * 64), 
			D3D12_RESOURCE_STATE_GENERIC_READ, 
			nullptr, 
			IID_PPV_ARGS(&mUploadBuffer));

		CD3DX12_RANGE readRange(0, 0);
		mUploadBuffer->Map(0, &readRange, reinterpret_cast<void**>(&mResource));
	}

	void DX12GFXConstantBuffer::SetDevice(DX12GFXDevice* device)
	{
		AssertNotNull(device);
		mDevice = device;
	}

	U32 DX12GFXConstantBuffer::GetNumElements() const
	{
		return mNumElements;
	}

	void DX12GFXConstantBuffer::SetData(void* data, U32 size, U32 objectIndex)
	{
		U32 alignedSize = MemoryUtils::AlignSize(size, 255);
		memcpy((U8*)mResource + (alignedSize * objectIndex), data, size);
	}

	ID3D12DescriptorHeap* DX12GFXConstantBuffer::GetDescriptorHeap()
	{
		return mDescriptorHeap.Get();
	}

	ID3D12Resource* DX12GFXConstantBuffer::GetResource()
	{
		return mUploadBuffer.Get();
	}

}