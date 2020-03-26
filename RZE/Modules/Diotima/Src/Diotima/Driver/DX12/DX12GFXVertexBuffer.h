#pragma once

#include <Diotima/Driver/GFXBuffer.h>

#include <Diotima/Driver/DX12/DX12.h>

namespace Diotima
{
	class DX12GFXDevice;

	class DX12GFXVertexBuffer final : public IGFXVertexBuffer
	{
	public:
		DX12GFXVertexBuffer() = default;
		virtual ~DX12GFXVertexBuffer() = default;

	public:
		virtual void Allocate(void* data, size_t size, U32 count) override;

	public:
		void SetDevice(DX12GFXDevice* device);

		D3D12_VERTEX_BUFFER_VIEW* GetBufferView();

		U32 GetNumElements() const;

	private:
		ComPtr<ID3D12Resource> mGPUBuffer;
		ComPtr<ID3D12Resource> mUploadBuffer;

		D3D12_VERTEX_BUFFER_VIEW* mGPUBufferView;
		DX12GFXDevice* mDevice;

		U32 mNumElements;
	};
}