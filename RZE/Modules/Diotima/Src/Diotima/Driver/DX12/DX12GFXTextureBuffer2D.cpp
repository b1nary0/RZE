#include <Diotima/Driver/DX12/DX12GFXTextureBuffer2D.h>

#include <Diotima/Driver/DX12/DX12GFXDevice.h>

#include <Utils/DebugUtils/Debug.h>

namespace Diotima
{
	void DX12GFXTextureBuffer2D::Allocate(void* data, U32 width, U32 height)
	{
		D3D12_RESOURCE_DESC resourceDesc = {};
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		resourceDesc.Alignment = 0;
		resourceDesc.Width = width;
		resourceDesc.Height = height;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.MipLevels = 1;
		resourceDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.SampleDesc.Quality = 0;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		mDevice->GetDevice()->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), 
			D3D12_HEAP_FLAG_NONE, 
			&resourceDesc, 
			D3D12_RESOURCE_STATE_COPY_DEST, 
			nullptr, 
			IID_PPV_ARGS(&mGPUBuffer));

		U64 uploadBufferRequiredSize = 0;
		mDevice->GetDevice()->GetCopyableFootprints(&resourceDesc, 0, 1, 0, nullptr, nullptr, nullptr, &uploadBufferRequiredSize);

		mDevice->GetDevice()->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), 
			D3D12_HEAP_FLAG_NONE, 
			&CD3DX12_RESOURCE_DESC::Buffer(uploadBufferRequiredSize), 
			D3D12_RESOURCE_STATE_GENERIC_READ, 
			nullptr, 
			IID_PPV_ARGS(&mUploadBuffer));

		D3D12_SUBRESOURCE_DATA textureData = {};
		textureData.pData = data;
		textureData.RowPitch = sizeof(float) * width;
		textureData.SlicePitch = textureData.RowPitch * height;

		ID3D12GraphicsCommandList* const commandList = mDevice->GetCommandList();
		UpdateSubresources(commandList, mGPUBuffer.Get(), mUploadBuffer.Get(), 0, 0, 1, &textureData);

		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mGPUBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
	}

	void DX12GFXTextureBuffer2D::SetDevice(DX12GFXDevice* device)
	{
		AssertNotNull(device);
		mDevice = device;
	}

}