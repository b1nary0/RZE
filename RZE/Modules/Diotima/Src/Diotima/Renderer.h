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

	// #TODO
	// This is just here as a reminder to figure this out.
	// This is how the driver state needs to be set to draw an item.
	// Might be a good idea to investigate using the opt-in/out defaults system.
	// If something hasn't been overridden or excluded, it will use a default state.
	// Then there is no state leakage because everything just sets what it needs.
	struct GraphicsState
	{

	};

	// Essentially a direct copy into a constant buffer. All this data is consumed by the GPU
	// for lighting etc
	struct MaterialData
	{
		float Shininess;
	};

	// Bundle of data that is necessary for each draw call.
	// #TODO
	// These will need to be wrapped or otherwise sorted properly by some higher-level design.
	// This is simply the data for any item with geometry. When and how this data is used is determined
	// elsewhere. The U32 handles should be designed such that 0 means no data. 
	// For now, each item will need a Vertex and Index buffer, but should be easily extensible
	// to support other types of data construction or use.
	struct RenderData
	{
		// This GraphicsState may not need to live on RenderData.
		GraphicsState DriverState;
		MaterialData Material;
		U32 VertexBuffer;
		U32 IndexBuffer;
		U32 TextureBuffer;
		U32 ConstantBuffer;
		Matrix4x4 Transform;
	};

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
		
		void* mWindowHandle;

		Vector2D mViewportDimensions;
	};
}
