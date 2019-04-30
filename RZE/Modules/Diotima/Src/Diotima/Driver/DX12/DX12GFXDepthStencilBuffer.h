#pragma once

#include <Diotima/Driver/GFXBuffer.h>

#include <Diotima/Driver/DX12/DX12.h>

namespace Diotima
{
	class DX12GFXDevice;

	class DX12GFXDepthStencilBuffer final : public IGFXDepthStencilBuffer
	{
	public:
		DX12GFXDepthStencilBuffer() = default;
		virtual ~DX12GFXDepthStencilBuffer() = default;

		virtual void Allocate() override;

	public:
		ID3D12Resource* GetResource();
		ID3D12DescriptorHeap* GetDescriptorHeap();
		ID3D12DescriptorHeap* GetTextureHeap();

		void SetDevice(DX12GFXDevice* device);
		
		CD3DX12_CPU_DESCRIPTOR_HANDLE mSRVCPUHandle;
		CD3DX12_GPU_DESCRIPTOR_HANDLE mSRVGPUHandle;
	private:
		DX12GFXDevice* mDevice;

		D3D12_DEPTH_STENCIL_VIEW_DESC mDSV;

		ComPtr<ID3D12Resource> mGPUBuffer;
		ComPtr<ID3D12DescriptorHeap>  mDescriptorHeap;
		ComPtr<ID3D12DescriptorHeap> mTextureHeap;
	};
}