#pragma once

#include <Rendering/Driver/GFXDevice.h>
#include <Rendering/Driver/OpenGL/GLShaderTranslator.h>

#include <Utils/PrimitiveDefs.h>

#include <array>
#include <map>
#include <mutex>
#include <set>
#include <tuple>
#include <vector>

struct SDL_Window;
struct SDL_GLContextState;

namespace Rendering
{
	class GLComputeShader;
	class GLPixelShader;
	class GLTextureBuffer2D;
	class GLVertexBuffer;
	class GLVertexShader;

	// OpenGL 4.5 core, drawn the way Direct3D draws so the engine and its shaders don't change. Depth runs 0 to 1, and
	// the engine's own targets have the clip space's origin at the upper left (glClipControl), so row 0 of each is its
	// top, as in D3D: viewports, texture coordinates and readback need no changes. Front faces are counter-clockwise on
	// screen, as D3D's rasterizer states have them, under either origin.
	//
	// The back buffer is the exception: it's drawn GL's way up (lower-left origin), as the window shows it, and Present
	// copies it onto the window as it is. What draws to it, the final full-screen copy (which samples by UV) and ImGui
	// (whose GL backend always draws GL's way up), comes out the right way up with no changes either.
	//
	// D3D binds textures to each stage's slots and samplers apart from them; GL binds a texture and a sampler together
	// to a unit, per program. So the device keeps what's bound to each stage's slots, as D3D's context does, and binds
	// them for each draw or dispatch where its programs read them (GLShaderTranslator).
	class GLDevice : public IGFXDevice
	{
	public:
		GLDevice() = default;
		~GLDevice() override = default;

	// IGFXDevice interface
	public:
		void SetWindow(void* windowHandle) override;
		void Initialize(bool validate) override;
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
		void GenerateMips(ITextureBuffer2D& texture, U32 firstSlice, U32 sliceCount) override;
		void CopyTexture(ITextureBuffer2D& source, ITextureBuffer2D& destination) override;
		void CopyTextureRegion(ITextureBuffer2D& source, U32 sourceX, U32 sourceY, U32 width, U32 height, ITextureBuffer2D& destination, U32 destinationX, U32 destinationY) override;
		void CopyTextureSlice(ITextureBuffer2D& source, U32 sourceSlice, ITextureBuffer2D& destination, U32 destinationSlice) override;
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
		void DrawIndexedInstanced(U32 indexCount, U32 instanceCount, U32 startInstance) override;
		void Dispatch(U32 groupsX, U32 groupsY, U32 groupsZ) override;

		void ReadTexel(ITextureBuffer2D& texture, U32 x, U32 y, TexelReadback& result) override;
		// Into a pixel buffer, which FinishReadbacks reads once a fence after the copy has passed
		void ReadTexture(ITextureBuffer2D& texture, const std::shared_ptr<TextureReadback>& result) override;
		void FinishReadbacks() override;
		void ReadBackBuffer(TextureReadback& result) override;

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

		// What the shaders' and resources' SetActive and SetInputLayout bind
		void SetVertexShader(const GLVertexShader& shader);
		void SetPixelShader(const GLPixelShader& shader);
		void SetComputeShader(const GLComputeShader& shader);
		void SetInputLayout(const GLVertexShader& shader);
		void SetVertexBuffer(const GLVertexBuffer& buffer, U32 slot);
		void SetIndexBuffer(U32 buffer);
		// texture may be null, which unbinds the slot
		void SetTextureResource(GLShaderTranslator::EStage stage, U32 slot, const GLTextureBuffer2D* texture);

	private:
		// D3D's texture registers per stage
		static constexpr U32 k_textureSlotCount = 16;
		// D3D's vertex buffer slots, each a GL vertex buffer binding
		static constexpr U32 k_vertexBufferSlotCount = 16;
		// GL_CLIP_DISTANCEi that can be enabled: GL's minimum, as many as D3D11's SV_ClipDistance
		static constexpr U32 k_clipDistanceCount = 8;
		// The four samplers the shaders' s registers mean (GLDevice.cpp's ESampler)
		static constexpr U32 k_samplerCount = 4;

		// A framebuffer with these attached: a colour texture, a depth texture, and which slice of the depth (-1 for all)
		using FramebufferKey = std::tuple<U32, U32, int>;

		// What DeleteLater was given, on the render thread
		void DeleteReleasedObjects();

		void CreateBackBuffer(U32 width, U32 height);
		void DestroyBackBuffer();
		// Which way up the bound target is drawn (GLDevice's comment)
		void SetIsBackBufferBound(bool isBackBufferBound);
		// The engine's targets, each a framebuffer made the first time it's asked for and kept
		U32 GetFramebuffer(const FramebufferKey& key);
		void BindFramebuffer(const FramebufferKey& key);

		// Binds stage's slots' textures and samplers where program reads them
		void BindTextures(const GLShaderTranslator::Result& program, const std::array<U32, k_textureSlotCount>& slots);
		// Before a draw: the graphics pipeline, its vertex input, its textures, and the vertex shader's clip distances
		void PrepareDraw();
		// After a draw: the clip distances off again, for what draws without the pipeline (ImGui's renderer), whose shaders
		// don't write them
		void FinishDraw();
		// Enables GL_CLIP_DISTANCE0 to count - 1 and disables the rest. D3D clips with what the vertex shader writes; GL
		// only with the distances enabled.
		void SetClipDistanceCount(U32 count);
		// Once per vertex and pixel shader drawn together: logs if the pixel shader reads an input where the vertex shader
		// doesn't write it, which D3D matches by semantic and GL by location
		void CheckVaryings();

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

		std::map<FramebufferKey, U32> m_framebuffers;

		// The vertex and pixel programs draws use, and the compute program dispatches use, bound as D3D binds a stage
		U32 m_graphicsPipeline = 0;
		U32 m_computePipeline = 0;
		const GLVertexShader* m_vertexShader = nullptr;
		const GLPixelShader* m_pixelShader = nullptr;
		const GLComputeShader* m_computeShader = nullptr;
		std::set<std::pair<U32, U32>> m_checkedVaryings;

		// The one vertex array, which the input layout and vertex and index buffers are set on
		U32 m_vertexArray = 0;
		U32 m_enabledAttributeMask = 0;
		// The vertex buffer bindings stepped once per instance (glVertexArrayBindingDivisor 1) rather than per vertex
		U32 m_instancedBindingMask = 0;
		U32 m_enabledClipDistanceMask = 0;
		U32 m_primitiveMode = 0;

		std::array<U32, k_samplerCount> m_samplers = {};
		std::array<U32, k_textureSlotCount> m_pixelTextures = {};
		std::array<U32, k_textureSlotCount> m_computeTextures = {};

		std::mutex m_releasedObjectsMutex;
		std::vector<std::pair<EObjectType, U32>> m_releasedObjects;

		U32 m_syncInterval = 0;
		// Set on the window's context at the next Present, on the render thread
		bool m_isSyncIntervalChanged = true;

		// A ReadTexture copy the GPU may not have made yet, and where it goes. Deleted by Shutdown if still waiting.
		struct PendingTextureReadback
		{
			U32 Buffer = 0;
			// The GLsync after the copy; void* so this header needs no GL header
			void* Fence = nullptr;
			// D24S8, whose texels go into D3D's order (GLDevice.cpp's ToD3DDepthStencil)
			bool IsDepth = false;
			std::shared_ptr<TextureReadback> Result;
		};
		std::vector<PendingTextureReadback> m_pendingTextureReadbacks;
	};
}
