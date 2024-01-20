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

class Vector2D;
class Vector4D;

namespace Rendering
{
	class DX11Device;
	class IVertexBuffer;
	class RenderTargetTexture;

	struct GFXTextureBufferParams;
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

		static void ImGuiRender();

	public:
		static VertexBufferHandle CreateVertexBuffer(void* data, size_t dataTypeSize, size_t count, U32 stride);
		static IndexBufferHandle CreateIndexBuffer(void* data, size_t dataTypeSize, size_t count);
		static ConstantBufferHandle CreateConstantBuffer(void* data, size_t dataTypeSize, size_t count);
		static TextureBuffer2DHandle CreateTextureBuffer2D(const void* data, const GFXTextureBufferParams& params);
		
		static VertexShaderHandle CreateVertexShader(const Filepath& filepath, const ShaderInputLayout& inputLayout);
		static PixelShaderHandle CreatePixelShader(const Filepath& filepath);

		static void UploadDataToBuffer(const ConstantBufferHandle& buffer, const void* data);

		static void ReleaseVertexShader(VertexShaderHandle& shaderHandle);
		static void ReleasePixelShader(PixelShaderHandle& shaderHandle);

		static void ClearDepthStencilBuffer(const TextureBuffer2DHandle& buffer);

	public:
		static void SetRenderTarget(const RenderTargetTexture* renderTarget);
		static void SetRenderTargetBackBuffer();

		static void ClearRenderTarget(const RenderTargetHandle& renderTarget, const Vector4D& colour);

		static void SetViewport(const ViewportParams& viewportParams);
		
		static void SetInputLayout(const VertexShaderHandle& vertexShader);
		static void SetPrimitiveTopology(EPrimitiveTopology topologyType);

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
		static RenderThread m_renderThread;
	};
}
