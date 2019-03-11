#include <Diotima/Driver/DX12/DX12GFXVertexBuffer.h>

#include <Diotima/Driver/DX12/DX12GFXDevice.h>

#include <Utils/DebugUtils/Debug.h>
#include <Utils/Math/Vector2D.h>
#include <Utils/Math/Vector3D.h>
#include <Utils/Math/Vector4D.h>

namespace Diotima
{
	struct TempDataLayoutStructure
	{
		Vector3D position;
 		Vector3D normal;
 		Vector2D uv;
 		Vector3D tangents;
	};

	struct TempVertexDataLayout
	{
		Vector3D position;
		Vector4D color;
	};

	void DX12GFXVertexBuffer::Allocate(void* data, U32 numElements)
	{
		mNumElements = numElements;

		size_t bufferSize = numElements * sizeof(float);
		ID3D12Device* device = mDevice->GetDevice();
		ID3D12GraphicsCommandList* commandList = mDevice->GetCommandList();
		ID3D12CommandQueue* commandQueue = mDevice->GetCommandQueue();

		HRESULT res = device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(bufferSize), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mUploadBuffer));
		res = device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(bufferSize), D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&mGPUBuffer));

		mDevice->ResetCommandList();

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

		//UpdateSubresources<1>(commandList, mGPUBuffer.Get(), mUploadBuffer.Get(), 0, 0, 1, &subResourceData);
		commandList->CopyResource(mGPUBuffer.Get(), mUploadBuffer.Get());

		commandList->ResourceBarrier(1,
			&CD3DX12_RESOURCE_BARRIER::Transition(mGPUBuffer.Get(),
				D3D12_RESOURCE_STATE_COPY_DEST,
				D3D12_RESOURCE_STATE_GENERIC_READ));

		commandList->Close();

		ID3D12CommandList* ppCommandLists[] = { commandList };
		commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

		// Initialize vertex buffer view
		mGPUBufferView = new D3D12_VERTEX_BUFFER_VIEW();
		mGPUBufferView->BufferLocation = mGPUBuffer->GetGPUVirtualAddress();
		mGPUBufferView->StrideInBytes = sizeof(TempDataLayoutStructure);
		mGPUBufferView->SizeInBytes = static_cast<UINT>(bufferSize);

		mDevice->WaitForPreviousFrame();
	}

	void DX12GFXVertexBuffer::SetDevice(DX12GFXDevice* device)
	{
		AssertNotNull(device);
		mDevice = device;
	}

	D3D12_VERTEX_BUFFER_VIEW* DX12GFXVertexBuffer::GetBufferView()
	{
		return mGPUBufferView;
	}

	U32 DX12GFXVertexBuffer::GetNumElements() const
	{
		return mNumElements;
	}

}