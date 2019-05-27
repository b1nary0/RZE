#include <Diotima/Driver/DX12/DX12GFXDepthStencilBuffer.h>

#include <Diotima/Driver/DX12/DX12GFXDevice.h>

#include <Utils/DebugUtils/Debug.h>

namespace Diotima
{
	constexpr U32 kWidth = 1584;
	constexpr U32 kHeight = 861;

	void DX12GFXDepthStencilBuffer::Allocate()
	{
		AssertExpr(mWidth != 0 && mHeight != 0);

		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
		dsvHeapDesc.NumDescriptors = 1;
		dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

		D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
		srvHeapDesc.NumDescriptors = 1;
		srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

		mDSV.Format = DXGI_FORMAT_D32_FLOAT;
		mDSV.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
		mDSV.Flags = D3D12_DSV_FLAG_NONE;

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
		srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Texture2D.MipLevels = 1;
		
		D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
		depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
		depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
		depthOptimizedClearValue.DepthStencil.Stencil = 0;

		mDevice->GetDevice()->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R32_TYPELESS, mWidth, mHeight, 1, 1, mDevice->GetMSAASampleCount(), 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&depthOptimizedClearValue,
			IID_PPV_ARGS(&mGPUBuffer));

		mDevice->GetDevice()->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&mDescriptorHeap));
		mDevice->GetDevice()->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mTextureHeap));

		mDevice->GetDevice()->CreateDepthStencilView(mGPUBuffer.Get(), &mDSV, mDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

		mSRVCPUHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mDevice->GetTextureHeap()->GetCPUDescriptorHandleForHeapStart());
		mSRVCPUHandle.Offset(512, mDevice->GetCBVSRVUAVDescriptorSize());

		mSRVGPUHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(mDevice->GetTextureHeap()->GetGPUDescriptorHandleForHeapStart());
		mSRVGPUHandle.Offset(512, mDevice->GetCBVSRVUAVDescriptorSize());

		mDevice->GetDevice()->CreateShaderResourceView(mGPUBuffer.Get(), &srvDesc, mSRVCPUHandle);
	}

	void DX12GFXDepthStencilBuffer::SetDevice(DX12GFXDevice* device)
	{
		AssertNotNull(device);
		mDevice = device;
	}

	ID3D12DescriptorHeap* DX12GFXDepthStencilBuffer::GetDescriptorHeap()
	{
		return mDescriptorHeap.Get();
	}

	ID3D12Resource* DX12GFXDepthStencilBuffer::GetResource()
	{
		return mGPUBuffer.Get();
	}

	ID3D12DescriptorHeap* DX12GFXDepthStencilBuffer::GetTextureHeap()
	{
		return mTextureHeap.Get();
	}

	void DX12GFXDepthStencilBuffer::SetSize(int newWidth, int newHeight)
	{
		AssertExpr(newWidth != 0 && newHeight != 0);

		mWidth = newWidth;
		mHeight = newHeight;
	}

}