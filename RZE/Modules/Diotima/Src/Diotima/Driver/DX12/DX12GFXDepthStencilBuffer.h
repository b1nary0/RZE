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

		void SetDevice(DX12GFXDevice* device);

	private:
		DX12GFXDevice* mDevice;

		ComPtr<ID3D12Resource> mGPUBuffer;
		ComPtr<ID3D12DescriptorHeap>  mDescriptorHeap;
	};
}