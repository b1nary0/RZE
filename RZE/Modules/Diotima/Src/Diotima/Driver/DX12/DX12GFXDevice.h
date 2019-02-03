#pragma once

#include <memory>

#include <wrl.h>
#include <d3d12.h>

#include <Diotima/Driver/GFXDevice.h>

using namespace Microsoft::WRL;

namespace Diotima
{
	class DX12GFXDevice : public IGFXDevice
	{
	public:
		DX12GFXDevice() = default;
		virtual ~DX12GFXDevice() = default;

	public:
		virtual void Initialize() final override;

	private:
		ComPtr<ID3D12Device> mDevice;
	};
}