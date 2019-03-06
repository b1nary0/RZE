#pragma once

#include <Diotima/Driver/GFXBuffer.h>

#include <Diotima/Driver/DX12/DX12.h>

namespace Diotima
{
	class DX12GFXDevice;

	class DX12GFXTextureBuffer2D final : public IGFXTextureBuffer2D
	{
	public:
		DX12GFXTextureBuffer2D() = default;
		virtual ~DX12GFXTextureBuffer2D() = default;

		virtual void Allocate(void* data, U32 width, U32 height) override;

	public:
		void SetDevice(DX12GFXDevice* device);

		ID3D12DescriptorHeap* GetDescriptorHeap();
		CD3DX12_CPU_DESCRIPTOR_HANDLE GetDescriptorHandleCPU();
		CD3DX12_GPU_DESCRIPTOR_HANDLE GetDescriptorHandleGPU();

	private:
		DX12GFXDevice* mDevice;

		CD3DX12_CPU_DESCRIPTOR_HANDLE mSRVCPUHandle;
		CD3DX12_GPU_DESCRIPTOR_HANDLE mSRVGPUHandle;

		ComPtr<ID3D12DescriptorHeap> mDescriptorHeap;
		ComPtr<ID3D12Resource> mGPUBuffer;
		ComPtr<ID3D12Resource> mUploadBuffer;
	};
}