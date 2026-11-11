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
		// validate: the graphics API checks every call (the D3D11 debug layer, an OpenGL debug context) and the device
		// logs what it reports, asserting on errors. It costs CPU time in every call.
		virtual void Initialize(bool validate) = 0;
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
		// Fills every mip of sliceCount slices from firstSlice from their most detailed one; 0 slices for every slice
		virtual void GenerateMips(ITextureBuffer2D& texture, U32 firstSlice, U32 sliceCount) = 0;
		virtual void CopyTexture(ITextureBuffer2D& source, ITextureBuffer2D& destination) = 0;
		virtual void CopyTextureRegion(ITextureBuffer2D& source, U32 sourceX, U32 sourceY, U32 width, U32 height, ITextureBuffer2D& destination, U32 destinationX, U32 destinationY) = 0;
		// The most detailed mip of one slice, whole, into a slice of a texture of the same size and format. Depth textures
		// too, which can only be copied whole.
		virtual void CopyTextureSlice(ITextureBuffer2D& source, U32 sourceSlice, ITextureBuffer2D& destination, U32 destinationSlice) = 0;
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
		// Per-instance vertex data (EDataClassification::PER_INSTANCE) is read from startInstance on
		virtual void DrawIndexedInstanced(U32 indexCount, U32 instanceCount, U32 startInstance) = 0;
		virtual void Dispatch(U32 groupsX, U32 groupsY, U32 groupsZ) = 0;

		// Reading back. ReadTexel and ReadBackBuffer wait for the GPU, then fill result and mark it ready.
		virtual void ReadTexel(ITextureBuffer2D& texture, U32 x, U32 y, TexelReadback& result) = 0;
		// The most detailed mip of the first slice, rows top first. Doesn't wait for the GPU: result is filled in and
		// marked ready by a later FinishReadbacks, once the GPU has drawn the texture. Never, if the device shuts down first.
		virtual void ReadTexture(ITextureBuffer2D& texture, const std::shared_ptr<TextureReadback>& result) = 0;
		// Fills in the ReadTexture results the GPU has caught up with, without waiting for the others. After each batch of
		// commands.
		virtual void FinishReadbacks() = 0;
		// The frame as it will be presented (RGBA8, rows top first): before Present, which may discard it
		virtual void ReadBackBuffer(TextureReadback& result) = 0;
	};
}
