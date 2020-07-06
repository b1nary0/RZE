#pragma once

#include <functional>
#include <mutex>
#include <queue>
#include <unordered_map>
#include <vector>

#include <Diotima/RenderCommands.h>

#include <Utils/Math/Matrix4x4.h>
#include <Utils/Math/Vector2D.h>
#include <Utils/PrimitiveDefs.h>

struct ID3D11InputLayout;

namespace Diotima
{
	class DX11GFXDevice;

	class GFXPassGraph;
	class RenderTargetTexture;

	class Renderer
	{
		// Constructors
	public:
		Renderer();
		~Renderer();

	public:
		void Initialize();
		void Update();
		void Render();
		void ShutDown();

	public:
		
		// #TODO(Josh::Really really don't like this, fix later)
		// #TODO
		// Take with new renderer
		void SetWindow(void* handle) { mWindowHandle = handle; }

		void EnableVsync(bool bEnable);

		const Vector2D& GetCanvasSize();
		void ResizeCanvas(const Vector2D& newSize);

		void SetViewportSize(const Vector2D& newSize);
		const Vector2D& GetViewportSize();

		// This sets the render target for the entire pipeline. We essentially render to this
		// and then send it to the back buffer. Will be reworked at some point.
		void SetRenderTarget(RenderTargetTexture* renderTarget);
		// Can return null for now to determine if we want to write directly to the backbuffer.
		// Will fix later since we should be able to have a more robust system WRT RTTs.
		RenderTargetTexture* GetRenderTarget();

	public:
		U32 QueueCreateVertexBufferCommand(void* data, size_t size, U32 count);
		U32 QueueCreateIndexBufferCommand(void* data, size_t size, U32 count);
		U32 QueueCreateTextureCommand(ECreateTextureBufferType bufferType, void* data, U32 width, U32 height);

		void QueueUpdateRenderItem(U32 itemID, const Matrix4x4& worldMtx);

		void ProcessCommands();

		DX11GFXDevice& GetDriverDevice();

	private:
		U32 CreateVertexBuffer(void* data, size_t size, U32 count);
		U32 CreateIndexBuffer(void* data, size_t size, U32 count);
		U32 CreateTextureBuffer2D(void* data, U32 width, U32 height);

	private:
		Vector2D mCanvasSize;

		std::queue<Int32> mFreeRenderListIndices;

		RenderTargetTexture* mRenderTarget;

	private:
		std::vector<CreateBufferRenderCommand> mVertexBufferCommandQueue;
		std::vector<CreateBufferRenderCommand> mIndexBufferCommandQueue;
		std::vector<CreateTextureBufferRenderCommand> mTextureBufferCommandQueue;
		std::vector<UpdateRenderItemWorldMatrixCommand> mUpdateRenderItemWorldMatrixCommandQueue;

		// DX12 Temp
	private:
		std::unique_ptr<DX11GFXDevice> mDevice;
		std::unique_ptr<GFXPassGraph> mPassGraph;
		
		void* mWindowHandle;

		Vector2D mViewportDimensions;
	};
}
