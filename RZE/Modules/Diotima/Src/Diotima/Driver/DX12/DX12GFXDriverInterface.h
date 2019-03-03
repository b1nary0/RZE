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
		virtual void Present() override;
		virtual void Shutdown() override;

		virtual U32 CreateVertexBuffer(void* data, U32 numElements) override;
		virtual U32 CreateIndexBuffer(void* data, U32 numElements) override;
		virtual U32 CreateTextureBuffer2D(void* data, U32 width, U32 height) override;

		void SetWindow(void* windowHandle) override;

		std::unique_ptr<DX12GFXDevice> mDevice;

	private:
		void* mWindowHandle;
	};
}