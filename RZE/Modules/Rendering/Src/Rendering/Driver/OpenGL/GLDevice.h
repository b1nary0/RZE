#pragma once

#include <Rendering/Driver/GFXDevice.h>

#include <Utils/PrimitiveDefs.h>

#include <mutex>
#include <vector>

struct SDL_Window;
struct SDL_GLContextState;

namespace Rendering
{
	class GLComputeShader;
	class GLPixelShader;
	class GLVertexShader;

	// OpenGL 4.5 core, drawn the way Direct3D draws so the engine and its shaders don't change. Depth runs 0 to 1, and
	// the engine's own targets have the clip space's origin at the upper left (glClipControl), so row 0 of each is its
	// top, as in D3D: viewports, texture coordinates and readback need no changes.
	//
	// The back buffer is the exception: it's drawn GL's way up (lower-left origin), as the window shows it, and Present
	// copies it onto the window as it is. What draws to it, the final full-screen copy (which samples by UV) and ImGui
	// (whose GL backend always draws GL's way up), comes out the right way up with no changes either.
	class GLDevice : public IGFXDevice
	{
	public:
		GLDevice() = default;
		~GLDevice() override = default;

	// IGFXDevice interface
	public:
		void SetWindow(void* windowHandle) override;
		void Initialize() override;
		void Shutdown() override;

		// The GL context is current on one thread at a time
		void AttachToCurrentThread() override;
		void DetachFromCurrentThread() override;

		std::shared_ptr<IVertexBuffer> CreateVertexBuffer() override;
		std::shared_ptr<IIndexBuffer> CreateIndexBuffer() override;
		std::shared_ptr<IConstantBuffer> CreateConstantBuffer(U32 alignment) override;
		std::shared_ptr<ITextureBuffer2D> CreateTextureBuffer2D() override;
		std::shared_ptr<IVertexShader> CreateVertexShader() override;
		std::shared_ptr<IPixelShader> CreatePixelShader() override;
		std::shared_ptr<IComputeShader> CreateComputeShader() override;

		void Present() override;
		void HandleWindowResize(U32 newWidth, U32 newHeight) override;
		void SetSyncInterval(U32 interval) override;
		// Presenting waits for the display itself (SDL_GL_SwapWindow), so there's never a slot to wait for
		bool WaitForPresentSlot(U32 timeoutMS) override;
		void BeginDrawSet() override;

		// KHR_debug groups, which RenderDoc and similar tools show
		bool IsProfilerAttached() override;
		void BeginProfilerEvent(const char* name) override;
		void EndProfilerEvent() override;

		void ClearDepthStencilBuffer(ITextureBuffer2D& depthTexture, bool everySlice, U32 slice) override;
		void ClearRenderTargets() override;
		void SetRenderTargetBackBuffer() override;
		void SetDepthTarget(ITextureBuffer2D& depthTexture, U32 slice) override;
		void SetColourTarget(ITextureBuffer2D& colourTarget, ITextureBuffer2D* depthTexture) override;
		void GenerateMips(ITextureBuffer2D& texture) override;
		void CopyTexture(ITextureBuffer2D& source, ITextureBuffer2D& destination) override;
		void CopyTextureRegion(ITextureBuffer2D& source, U32 sourceX, U32 sourceY, U32 width, U32 height, ITextureBuffer2D& destination, U32 destinationX, U32 destinationY) override;
		void ClearRenderTarget(ITextureBuffer2D& renderTarget, const Vector4D& colour) override;

		void SetViewport(const ViewportParams& params) override;
		void SetPrimitiveTopology(EPrimitiveTopology topology) override;
		void SetRasterizerState(ERasterizerState state) override;
		void SetBlendState(EBlendState state) override;
		void SetDepthState(EDepthState state) override;
		void UnsetPixelShader() override;
		void UnsetTextureResource(U32 textureSlot) override;
		void UnsetTextureResourceCS(U32 textureSlot) override;
		void SetUnorderedAccess(ITextureBuffer2D& texture, U32 slot, U32 mip) override;
		void UnsetUnorderedAccess(U32 slot) override;

		void Draw(U32 vertexCount) override;
		void DrawIndexed(U32 indexCount) override;
		void Dispatch(U32 groupsX, U32 groupsY, U32 groupsZ) override;

		void ReadTexel(ITextureBuffer2D& texture, U32 x, U32 y, TexelReadback& result) override;
		void ReadTexture(ITextureBuffer2D& texture, TextureReadback& result) override;

	public:
		// For this backend's resources and shaders

		// A GL object to delete. Resources can be let go on any thread, the main thread among them, which has no GL
		// context, so they're deleted on the render thread, at the next Present.
		enum class EObjectType
		{
			Program,
			Buffer,
			Texture,
			Renderbuffer,
			Framebuffer,
			Sampler,
			VertexArray
		};
		// Safe from any thread
		void DeleteLater(EObjectType type, U32 name);

		// What the shaders' SetActive and SetInputLayout bind
		void SetVertexShader(const GLVertexShader& shader);
		void SetPixelShader(const GLPixelShader& shader);
		void SetComputeShader(const GLComputeShader& shader);
		void SetInputLayout(const GLVertexShader& shader);

	private:
		// What DeleteLater was given, on the render thread
		void DeleteReleasedObjects();

		void CreateBackBuffer(U32 width, U32 height);
		void DestroyBackBuffer();
		// Which way up the bound target is drawn (GLDevice's comment)
		void SetIsBackBufferBound(bool isBackBufferBound);

	private:
		SDL_Window* m_window = nullptr;
		SDL_GLContextState* m_context = nullptr;

		// What the frame draws into before Present puts it on the window (GLDevice's comment)
		U32 m_backBufferFramebuffer = 0;
		U32 m_backBufferColour = 0;
		U32 m_backBufferDepthStencil = 0;
		U32 m_backBufferWidth = 0;
		U32 m_backBufferHeight = 0;
		bool m_isBackBufferBound = false;

		// The vertex and pixel programs draws use, and the compute program dispatches use, bound as D3D binds a stage
		U32 m_graphicsPipeline = 0;
		U32 m_computePipeline = 0;
		const GLVertexShader* m_vertexShader = nullptr;
		const GLPixelShader* m_pixelShader = nullptr;
		const GLComputeShader* m_computeShader = nullptr;
		const GLVertexShader* m_inputLayout = nullptr;

		std::mutex m_releasedObjectsMutex;
		std::vector<std::pair<EObjectType, U32>> m_releasedObjects;

		U32 m_syncInterval = 0;
		// Set on the window's context at the next Present, on the render thread
		bool m_isSyncIntervalChanged = true;
	};
}
