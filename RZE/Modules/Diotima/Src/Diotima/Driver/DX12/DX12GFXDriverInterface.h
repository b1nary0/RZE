#pragma once

#include <memory>

#include <Diotima/Driver/GFXDriverInterface.h>

namespace Diotima
{
	class DX12GFXDevice;
	class DX12GFXTextureBuffer2D;

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
		virtual U32 CreateConstantBuffer(void* data, U32 size) override;

		// #TODO(Josh::Tease this out later)
		void GenerateMipsForTexture(DX12GFXTextureBuffer2D* texture);

		void SetWindow(void* windowHandle) override;

		std::unique_ptr<DX12GFXDevice> mDevice;

	private:
		void* mWindowHandle;
	};
}