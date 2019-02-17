#pragma once

#include <memory>

#include <Diotima/Driver/GFXDriverInterface.h>

namespace Diotima
{
	class DX12GFXDevice;

	class DX12GFXDriverInterface final : public IGFXDriverInterface
	{
	public:
		DX12GFXDriverInterface() = default;
		virtual ~DX12GFXDriverInterface() = default;

	public:
		virtual void Initialize() override;
		virtual void Shutdown() override;

		virtual void Present() override;

		void SetWindow(void* windowHandle) override;

	private:
		std::unique_ptr<DX12GFXDevice> mDevice;
		void* mWindowHandle;
	};
}