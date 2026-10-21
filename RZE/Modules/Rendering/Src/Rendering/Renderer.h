#pragma once

#include <Rendering/RenderingDefines.h>
#include <Rendering/BufferHandle.h>
#include <Rendering/ShaderHandle.h>
#include <Rendering/RenderCommand.h>

#include <Rendering/RenderThread.h>

#include <Utils/PrimitiveDefs.h>
#include <Utils/Platform/Filepath.h>

#include <memory>
#include <queue>
#include <vector>

class Vector2D;
class Vector4D;

namespace Rendering
{
	class DX11Device;
	class IVertexBuffer;

	class UIFrameData;

	struct GFXTextureBufferParams;
	struct TexelReadback;
	struct TextureReadback;
	struct ViewportParams;
	
	class Renderer
	{
	private:
		struct RenderState
		{
			// This will get filled out per Begin/End call
		};

		// Constructors
	public:
		Renderer();
		~Renderer();
		
	public:
		static void Initialize(void* mWindowHandle);
		static void Shutdown();

		// API for an entire frame (global state, etc)
		static void BeginFrame(const char* frameName);
		static void EndFrame();

		// API for a render pass (series of draw calls)
		static void Begin(const char* drawSetName);
		static void End();

		static void DevicePresent();

		static void HandleWindowResize(const Vector2D& newSize);

		// How many vertical blanks each present waits for: 0 presents at once (vsync off), 1 on every refresh (vsync on).
		// Takes effect from the next frame the render thread runs.
		static void SetSyncInterval(U32 interval);
		// Blocks until the swap chain can queue another frame, which keeps at most two frames in flight, or until
		// timeoutMS passes (false), e.g. while the window is minimised. Call once per frame, before reading its input.
		static bool WaitForPresentSlot(U32 timeoutMS);

		static void ImGuiRender();
		// Keeps texture alive while the UI being drawn now is rendered, which goes on until a newer UI replaces it. ImGui
		// only holds a texture's view as a raw pointer, so call it for each texture the UI draws. Main thread only.
		static void KeepTextureForUI(const TextureBuffer2DHandle& texture);

	public:
		static VertexBufferHandle CreateVertexBuffer(const void* data, size_t dataTypeSize, size_t count, U32 stride);
		// Created empty; contents are written with UpdateVertexBuffer()
		static VertexBufferHandle CreateDynamicVertexBuffer(size_t capacityBytes, U32 stride);
		static IndexBufferHandle CreateIndexBuffer(const void* data, size_t dataTypeSize, size_t count);
		static ConstantBufferHandle CreateConstantBuffer(void* data, size_t dataTypeSize, U32 alignment, size_t count);
		// Without contents, e.g. a render target
		static TextureBuffer2DHandle CreateTextureBuffer2D(const GFXTextureBufferParams& params);
		// Uploads texels, which are freed once the render thread has made the texture
		static TextureBuffer2DHandle CreateTextureBuffer2D(TexelBuffer&& texels, const GFXTextureBufferParams& params);
		
		static VertexShaderHandle CreateVertexShader(const Filepath& filepath, const ShaderInputLayout& inputLayout);
		static PixelShaderHandle CreatePixelShader(const Filepath& filepath);

		template <typename TType>
		static void UploadDataToBuffer(const ConstantBufferHandle& buffer, const void* data);
		// Overwrites a dynamic vertex buffer's contents. size must not exceed the buffer's capacity.
		static void UpdateVertexBuffer(const VertexBufferHandle& buffer, const void* data, size_t size);

		static void ReleaseVertexShader(VertexShaderHandle& shaderHandle);
		static void ReleasePixelShader(PixelShaderHandle& shaderHandle);

		static void ClearDepthStencilBuffer(const TextureBuffer2DHandle& buffer);

	public:
		// Unbinds every colour and depth target, e.g. so a target can be read from next
		static void ClearRenderTargets();
		static void SetRenderTargetBackBuffer();
		// Binds only a depth buffer (no colour target), e.g. for shadow map rendering
		static void SetDepthTarget(const TextureBuffer2DHandle& depthTexture);
		// Binds a single colour texture, optionally with a depth buffer
		static void SetColourTarget(const RenderTargetHandle& colourTarget);
		static void SetColourTarget(const RenderTargetHandle& colourTarget, const TextureBuffer2DHandle& depthTexture);

		// Fills the texture's mip chain from mip 0
		static void GenerateMips(const TextureBuffer2DHandle& texture);
		// Copies one texel of a 4-byte-per-texel texture back to the CPU once the GPU has drawn it. The render
		// thread waits on the GPU for it, so keep it to occasional reads such as picking.
		static void ReadTexel(const TextureBuffer2DHandle& texture, U32 x, U32 y, std::shared_ptr<TexelReadback> result);
		// Copies the whole of a texture's most detailed mip back to the CPU once the GPU has drawn it. Like ReadTexel,
		// the render thread waits on the GPU for it, so keep it to one-off reads such as saving a capture to a file.
		static void ReadTexture(const TextureBuffer2DHandle& texture, std::shared_ptr<TextureReadback> result);

		static void ClearRenderTarget(const RenderTargetHandle& renderTarget, const Vector4D& colour);

		static void SetViewport(const ViewportParams& viewportParams);
		
		static void SetInputLayout(const VertexShaderHandle& vertexShader);
		static void SetPrimitiveTopology(EPrimitiveTopology topologyType);
		// Reset to Default by every Begin()
		static void SetRasterizerState(ERasterizerState state);
		// Stays bound until changed, so set it back to Opaque after drawing
		static void SetBlendState(EBlendState state);
		// Stays bound until changed, so set it back to ReadWrite after drawing
		static void SetDepthState(EDepthState state);

		static void SetVertexShader(const VertexShaderHandle& vertexShader);
		static void SetPixelShader(const PixelShaderHandle& pixelShader);

		static void SetConstantBufferVS(const ConstantBufferHandle& buffer, U32 bufferSlot);
		static void SetConstantBufferPS(const ConstantBufferHandle& buffer, U32 bufferSlot);

		static void SetVertexBuffer(const VertexBufferHandle& buffer, U32 bufferSlot);
		static void SetIndexBuffer(const IndexBufferHandle& buffer);

		static void SetTextureResource(const TextureBuffer2DHandle& texture, U32 textureSlot);
		static void UnsetTextureResource(U32 textureSlot);

		static void Draw(const VertexBufferHandle& vertexBuffer, size_t count);
		static void DrawIndexed(const IndexBufferHandle& indexBuffer);
		static void DrawFullScreenQuad();

	private:
		static void InternalUploadDataToBuffer(const ConstantBufferHandle& buffer, const void* data, size_t dataSize);

	private:
		static RenderThread m_renderThread;
		// The last UI frame's copy of ImGui's draw data, drawn again until ImGui begins a new frame
		static std::shared_ptr<UIFrameData> m_uiFrameData;
		// ImGui's frame count when m_uiFrameData was copied
		static int m_uiFrameNumber;
		// The textures the UI frame being drawn draws, which m_uiFrameData takes when it's copied
		static std::vector<TextureBuffer2DHandle> m_uiTextures;
	};

	template<typename TType>
	inline void Renderer::UploadDataToBuffer(const ConstantBufferHandle& buffer, const void* data)
	{
		InternalUploadDataToBuffer(buffer, data, sizeof(TType));
	}
}
