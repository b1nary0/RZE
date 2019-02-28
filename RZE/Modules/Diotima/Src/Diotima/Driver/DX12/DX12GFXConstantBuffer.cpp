#pragma once
#include <Diotima/Driver/DX12/DX12GFXConstantBuffer.h>

#include <Diotima/Driver/DX12/DX12GFXDevice.h>

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

		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
		heapDesc.NumDescriptors = 1;
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		mDevice->GetDevice()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&mDescriptorHeap));

		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation = mUploadBuffer->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes = ((sizeof(float) * numElements) + 255) & ~255;
		mDevice->GetDevice()->CreateConstantBufferView(&cbvDesc, mDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

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

	void DX12GFXConstantBuffer::SetData(void* data)
	{
		memcpy(mResource, data, sizeof(float) * mNumElements);
	}

	ID3D12DescriptorHeap* DX12GFXConstantBuffer::GetDescriptorHeap()
	{
		return mDescriptorHeap.Get();
	}

}