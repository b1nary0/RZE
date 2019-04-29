#include <Diotima/Driver/DX12/DX12GFXIndexBuffer.h>

#include <Diotima/Driver/DX12/DX12GFXDevice.h>

#include <Utils/DebugUtils/Debug.h>

namespace Diotima
{

	void DX12GFXIndexBuffer::Allocate(void* data, U32 numElements)
	{
		mNumElements = numElements;

		size_t bufferSize = numElements * sizeof(U32);
		ID3D12Device* device = mDevice->GetDevice();

		HRESULT res = device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(bufferSize), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mUploadBuffer));
		res = device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(bufferSize), D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&mGPUBuffer));

		ID3D12GraphicsCommandList* commandList = mDevice->GetGraphicsCommandList(mDevice->CreateGraphicsCommandList(mDevice->GetResourceCommandAllocator(), nullptr));

		commandList->ResourceBarrier(1,
			&CD3DX12_RESOURCE_BARRIER::Transition(mGPUBuffer.Get(),
				D3D12_RESOURCE_STATE_COMMON,
				D3D12_RESOURCE_STATE_COPY_DEST));

		// Copy to buffer
		U8* pVertexDataBegin;
		CD3DX12_RANGE readRange(0, 0);
		mUploadBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));
		memcpy(pVertexDataBegin, data, bufferSize);
		mUploadBuffer->Unmap(0, nullptr);

		D3D12_SUBRESOURCE_DATA subResourceData = {};
		subResourceData.pData = pVertexDataBegin;
		subResourceData.RowPitch = bufferSize;
		subResourceData.SlicePitch = subResourceData.RowPitch;

		UpdateSubresources<1>(commandList, mGPUBuffer.Get(), mUploadBuffer.Get(), 0, 0, 1, &subResourceData);

		commandList->ResourceBarrier(1,
			&CD3DX12_RESOURCE_BARRIER::Transition(mGPUBuffer.Get(),
				D3D12_RESOURCE_STATE_COPY_DEST,
				D3D12_RESOURCE_STATE_GENERIC_READ));

		mDevice->ExecuteCommandList(commandList);

		// Initialize vertex buffer view
		mGPUBufferView = new D3D12_INDEX_BUFFER_VIEW();
		mGPUBufferView->Format = DXGI_FORMAT_R32_UINT;
		mGPUBufferView->BufferLocation = mGPUBuffer->GetGPUVirtualAddress();
		mGPUBufferView->SizeInBytes = static_cast<UINT>(bufferSize);

		mDevice->WaitForPreviousFrame();
	}

	void DX12GFXIndexBuffer::SetDevice(DX12GFXDevice* device)
	{
		AssertNotNull(device);
		mDevice = device;
	}

	U32 DX12GFXIndexBuffer::GetNumElements() const
	{
		return mNumElements;
	}

	D3D12_INDEX_BUFFER_VIEW* DX12GFXIndexBuffer::GetBufferView()
	{
		return mGPUBufferView;
	}

}