#pragma once

#include <Diotima/Driver/GFXBuffer.h>

#include <Diotima/Driver/DX12/DX12.h>

namespace Diotima
{
	class DX12GFXDevice;

	class DX12GFXIndexBuffer final : public IGFXIndexBuffer
	{
	public:
		DX12GFXIndexBuffer() = default;
		virtual ~DX12GFXIndexBuffer() = default;

	public:
		virtual void Allocate(void* data, U32 numElements) override;

	public:
		void SetDevice(DX12GFXDevice* device);

		D3D12_INDEX_BUFFER_VIEW* GetBufferView();

		U32 GetNumElements() const;

	private:
		ComPtr<ID3D12Resource> mGPUBuffer;
		ComPtr<ID3D12Resource> mUploadBuffer;

		DX12GFXDevice* mDevice;
		D3D12_INDEX_BUFFER_VIEW* mGPUBufferView;

		U32 mNumElements;
	};
}