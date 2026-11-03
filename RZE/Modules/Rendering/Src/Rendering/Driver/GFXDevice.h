#pragma once

#include <Rendering/RenderingDefines.h>

#include <Utils/PrimitiveDefs.h>

#include <memory>

class Vector4D;

namespace Rendering
{
	class IVertexBuffer;
	class IIndexBuffer;
	class IConstantBuffer;
	class ITextureBuffer2D;
	class IVertexShader;
	class IPixelShader;
	class IComputeShader;
	struct TexelReadback;
	struct TextureReadback;

	// A graphics API, as the render thread uses it: the device and what's bound to it. One per process, made by
	// GFXDeviceFactory; each graphics API has its own (Driver/<API>/). What a resource does to itself (allocating,
	// binding itself to a slot) is on the resource's interface (GFXBuffer.h, ShaderTypes.h) instead.
	//
	// The render thread calls everything but the Create functions and WaitForPresentSlot, after attaching the device to
	// itself. Resources passed in must have been made by this device.
	class IGFXDevice
	{
	public:
		IGFXDevice() = default;
		virtual ~IGFXDevice() = default;

	public:
		// The window's SDL_Window, which the device presents to; before Initialize
		virtual void SetWindow(void* windowHandle) = 0;
		virtual void Initialize() = 0;
		virtual void Shutdown() = 0;

		// The device can only be used from one thread at a time on some APIs (e.g. OpenGL's context): the thread that
		// made it until it detaches, then whichever thread attaches it
		virtual void AttachToCurrentThread() = 0;
		virtual void DetachFromCurrentThread() = 0;

		// Resources, made empty: they're allocated or created later, on the render thread. Safe on the main thread.
		virtual std::shared_ptr<IVertexBuffer> CreateVertexBuffer() = 0;
		virtual std::shared_ptr<IIndexBuffer> CreateIndexBuffer() = 0;
		virtual std::shared_ptr<IConstantBuffer> CreateConstantBuffer(U32 alignment) = 0;
		virtual std::shared_ptr<ITextureBuffer2D> CreateTextureBuffer2D() = 0;
		virtual std::shared_ptr<IVertexShader> CreateVertexShader() = 0;
		virtual std::shared_ptr<IPixelShader> CreatePixelShader() = 0;
		virtual std::shared_ptr<IComputeShader> CreateComputeShader() = 0;

		// The frame
		virtual void Present() = 0;
		virtual void HandleWindowResize(U32 newWidth, U32 newHeight) = 0;
		// 0 presents at once (tearing where the system allows it), 1 on every vertical blank, 2 on every other, and so on
		virtual void SetSyncInterval(U32 interval) = 0;
		// Blocks until another frame can be queued for presenting, or timeoutMS passes (false). Safe from any thread.
		virtual bool WaitForPresentSlot(U32 timeoutMS) = 0;
		// The state every draw set starts with: the default rasterizer state and the shared samplers
		virtual void BeginDrawSet() = 0;

		// Named regions for PIX, RenderDoc and similar tools
		virtual bool IsProfilerAttached() = 0;
		virtual void BeginProfilerEvent(const char* name) = 0;
		virtual void EndProfilerEvent() = 0;

		// Targets
		// Every slice of a texture array, or only slice
		virtual void ClearDepthStencilBuffer(ITextureBuffer2D& depthTexture, bool everySlice, U32 slice) = 0;
		virtual void ClearRenderTargets() = 0;
		virtual void SetRenderTargetBackBuffer() = 0;
		virtual void SetDepthTarget(ITextureBuffer2D& depthTexture, U32 slice) = 0;
		// depthTexture may be null
		virtual void SetColourTarget(ITextureBuffer2D& colourTarget, ITextureBuffer2D* depthTexture) = 0;
		virtual void GenerateMips(ITextureBuffer2D& texture) = 0;
		virtual void CopyTexture(ITextureBuffer2D& source, ITextureBuffer2D& destination) = 0;
		virtual void CopyTextureRegion(ITextureBuffer2D& source, U32 sourceX, U32 sourceY, U32 width, U32 height, ITextureBuffer2D& destination, U32 destinationX, U32 destinationY) = 0;
		virtual void ClearRenderTarget(ITextureBuffer2D& renderTarget, const Vector4D& colour) = 0;

		// Pipeline state
		virtual void SetViewport(const ViewportParams& params) = 0;
		virtual void SetPrimitiveTopology(EPrimitiveTopology topology) = 0;
		virtual void SetRasterizerState(ERasterizerState state) = 0;
		virtual void SetBlendState(EBlendState state) = 0;
		virtual void SetDepthState(EDepthState state) = 0;
		virtual void UnsetPixelShader() = 0;
		virtual void UnsetTextureResource(U32 textureSlot) = 0;
		virtual void UnsetTextureResourceCS(U32 textureSlot) = 0;
		virtual void SetUnorderedAccess(ITextureBuffer2D& texture, U32 slot, U32 mip) = 0;
		virtual void UnsetUnorderedAccess(U32 slot) = 0;

		// Draws and compute
		virtual void Draw(U32 vertexCount) = 0;
		virtual void DrawIndexed(U32 indexCount) = 0;
		virtual void Dispatch(U32 groupsX, U32 groupsY, U32 groupsZ) = 0;

		// Reading back: waits for the GPU, then fills result and marks it ready
		virtual void ReadTexel(ITextureBuffer2D& texture, U32 x, U32 y, TexelReadback& result) = 0;
		virtual void ReadTexture(ITextureBuffer2D& texture, TextureReadback& result) = 0;
	};
}
