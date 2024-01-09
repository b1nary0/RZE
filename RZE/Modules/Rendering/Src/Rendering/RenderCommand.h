#pragma once

#include <Rendering/Rendering.h>
#include <Rendering/BufferHandle.h>
#include <Rendering/ShaderHandle.h>
#include <Rendering/Driver/ShaderTypes.h>
#include <Rendering/Driver/GFXBuffer.h>

#include <Utils/PrimitiveDefs.h>
#include <Utils/Math/Vector2D.h>
#include <Utils/Math/Vector4D.h>
#include <Utils/Platform/FilePath.h>

#include <memory>

namespace Rendering
{
	class DX11VertexBuffer;
	class DX11IndexBuffer;
	class DX11ConstantBuffer;
	class DX11TextureBuffer2D;
	class DX11VertexShader;
	class DX11PixelShader;
	class DX11Device;

	class RenderTargetTexture;

	enum RenderCommandType
	{
		Unset = 0,
		ImGuiRender,
		BeginFrame = 400,
		EndFrame,
		Begin,
		End,
		DevicePresent,
		HandleWindowResize,
		CreateVertexBuffer = 500,
		CreateIndexBuffer,
		CreateConstantBuffer,
		CreateTextureBuffer2D,
		CreateVertexShader,
		CreatePixelShader,
		UploadDataToBuffer,
		ReleaseVertexShader,
		ReleasePixelShader,
		ClearDepthStencilBuffer,
		SetRenderTarget,
		ClearRenderTargets,
		SetRenderTargetBackBuffer,
		ClearRenderTarget,
		SetViewport,
		SetInputLayout,
		SetPrimitiveTopology,
		SetVertexShader,
		SetPixelShader,
		SetConstantBufferVS,
		SetConstantBufferPS,
		SetVertexBuffer,
		SetIndexBuffer,
		SetTextureResource,
		UnsetTextureResource,
		Draw,
		DrawIndexed,
		DrawFullScreenQuad
	};

	// @todo a dummy class for storing these all separately just to prove out system.
	// doubt it needs to be inherited just for storage but i dont wanna think about it atm
	struct RenderCommand
	{
		RenderCommandType type;
	};

	struct RenderCommand_ImGuiRender : RenderCommand
	{
		RenderCommand_ImGuiRender() { type = RenderCommandType::ImGuiRender; }
	};

	struct RenderCommand_BeginFrame : RenderCommand
	{
		RenderCommand_BeginFrame() { type = RenderCommandType::BeginFrame; }

		const char* frameName = nullptr;
	};

	struct RenderCommand_EndFrame : RenderCommand
	{
		RenderCommand_EndFrame() { type = RenderCommandType::EndFrame; }
	};

	struct RenderCommand_Begin : RenderCommand
	{
		RenderCommand_Begin() { type = RenderCommandType::Begin; }

		const char* drawSetName = nullptr;
	};

	struct RenderCommand_End : RenderCommand
	{
		RenderCommand_End() { type = RenderCommandType::End; }
	};

	struct RenderCommand_DevicePresent : RenderCommand
	{
		RenderCommand_DevicePresent() { type = RenderCommandType::DevicePresent; }
	};

	struct RenderCommand_HandleWindowResize : RenderCommand
	{
		RenderCommand_HandleWindowResize() { type = RenderCommandType::HandleWindowResize; }

		Vector2D size;
	};

	struct RenderCommand_CreateVertexBuffer : RenderCommand
	{
		RenderCommand_CreateVertexBuffer() { type = RenderCommandType::CreateVertexBuffer; }

		size_t dataTypeSize = 0;
		U32 stride = 0;
		size_t count = 0;
		void* data = nullptr;
		// @todo could we not pass VertexBufferHandle here instead to avoid having
		// dx11 in this file
		std::shared_ptr<DX11VertexBuffer> bufferPtr = nullptr;
	};

	struct RenderCommand_CreateIndexBuffer : RenderCommand
	{
		RenderCommand_CreateIndexBuffer() { type = RenderCommandType::CreateIndexBuffer; }

		size_t dataTypeSize = 0;
		size_t count = 0;
		void* data = nullptr;
		std::shared_ptr<DX11IndexBuffer> bufferPtr = nullptr;
	};

	struct RenderCommand_CreateConstantBuffer : RenderCommand
	{
		RenderCommand_CreateConstantBuffer() { type = RenderCommandType::CreateConstantBuffer; }

		size_t dataTypeSize = 0;
		size_t count = 0;
		void* data = nullptr;
		std::shared_ptr<DX11ConstantBuffer> bufferPtr = nullptr;
	};

	struct RenderCommand_CreateTextureBuffer2D : RenderCommand
	{
		RenderCommand_CreateTextureBuffer2D() { type = RenderCommandType::CreateIndexBuffer; }

		GFXTextureBufferParams params;
		const void* data = nullptr;
		std::shared_ptr<DX11TextureBuffer2D> bufferPtr = nullptr;
	};

	struct RenderCommand_CreateVertexShader : RenderCommand
	{
		RenderCommand_CreateVertexShader() { type = RenderCommandType::CreateVertexShader; }

		Filepath filepath;
		ShaderInputLayout shaderInputLayout;
		std::shared_ptr<DX11VertexShader> bufferPtr = nullptr;
	};

	struct RenderCommand_CreatePixelShader : RenderCommand
	{
		RenderCommand_CreatePixelShader() { type = RenderCommandType::CreatePixelShader; }

		Filepath filepath;
		std::shared_ptr<DX11PixelShader> bufferPtr = nullptr;
	};

	struct RenderCommand_UploadDataToBuffer : RenderCommand
	{
		RenderCommand_UploadDataToBuffer() { type = RenderCommandType::UploadDataToBuffer; }

		const void* data = nullptr;
		ConstantBufferHandle bufferHandle;
	};

	struct RenderCommand_ReleaseVertexShader : RenderCommand
	{
		RenderCommand_ReleaseVertexShader() { type = RenderCommandType::ReleaseVertexShader; }

		VertexShaderHandle shaderHandle;
	};

	struct RenderCommand_ReleasePixelShader : RenderCommand
	{
		RenderCommand_ReleasePixelShader() { type = RenderCommandType::ReleasePixelShader; }

		PixelShaderHandle shaderHandle;
	};

	struct RenderCommand_ClearDepthStencilBuffer : RenderCommand
	{
		RenderCommand_ClearDepthStencilBuffer() { type = RenderCommandType::ClearDepthStencilBuffer; }

		TextureBuffer2DHandle bufferHandle;
	};

	struct RenderCommand_SetRenderTarget : RenderCommand
	{
		RenderCommand_SetRenderTarget() { type = RenderCommandType::SetRenderTarget; }

		const RenderTargetTexture* renderTarget;
	};

	struct RenderCommand_ClearRenderTargets : RenderCommand
	{
		RenderCommand_ClearRenderTargets() { type = RenderCommandType::ClearRenderTargets; }
	};

	struct RenderCommand_SetRenderTargetBackBuffer : RenderCommand
	{
		RenderCommand_SetRenderTargetBackBuffer() { type = RenderCommandType::SetRenderTargetBackBuffer; }
	};

	struct RenderCommand_ClearRenderTarget : RenderCommand
	{
		RenderCommand_ClearRenderTarget() { type = RenderCommandType::ClearRenderTarget; }

		RenderTargetHandle renderTarget;
		Vector4D colour;
	};
	
	struct RenderCommand_SetViewport : RenderCommand
	{
		RenderCommand_SetViewport() { type = RenderCommandType::SetViewport; }

		ViewportParams params;
	};

	struct RenderCommand_SetInputLayout : RenderCommand
	{
		RenderCommand_SetInputLayout() { type = RenderCommandType::SetInputLayout; }

		VertexShaderHandle shaderHandle;
	};

	struct RenderCommand_SetPrimitiveTopology : RenderCommand
	{
		RenderCommand_SetPrimitiveTopology() { type = RenderCommandType::SetPrimitiveTopology; }

		EPrimitiveTopology topology;
	};

	struct RenderCommand_SetVertexShader : RenderCommand
	{
		RenderCommand_SetVertexShader() { type = RenderCommandType::SetVertexShader; }

		VertexShaderHandle shaderHandle;
	};

	struct RenderCommand_SetPixelShader : RenderCommand
	{
		RenderCommand_SetPixelShader() { type = RenderCommandType::SetPixelShader; }

		PixelShaderHandle shaderHandle;
	};

	struct RenderCommand_SetConstantBufferVS : RenderCommand
	{
		RenderCommand_SetConstantBufferVS() { type = RenderCommandType::SetConstantBufferVS; }

		ConstantBufferHandle bufferHandle;
		U32 bufferSlot;
	};

	struct RenderCommand_SetConstantBufferPS : RenderCommand
	{
		RenderCommand_SetConstantBufferPS() { type = RenderCommandType::SetConstantBufferPS; }

		ConstantBufferHandle bufferHandle;
		U32 bufferSlot;
	};

	struct RenderCommand_SetVertexBuffer : RenderCommand
	{
		RenderCommand_SetVertexBuffer() { type = RenderCommandType::SetVertexBuffer; }

		VertexBufferHandle bufferHandle;
		U32 bufferSlot;
	};

	struct RenderCommand_SetIndexBuffer : RenderCommand
	{
		RenderCommand_SetIndexBuffer() { type = RenderCommandType::SetIndexBuffer; }

		IndexBufferHandle bufferHandle;
	};

	struct RenderCommand_SetTextureResource : RenderCommand
	{
		RenderCommand_SetTextureResource() { type = RenderCommandType::SetTextureResource; }

		TextureBuffer2DHandle bufferHandle;
		U32 bufferSlot;
	};

	struct RenderCommand_UnsetTextureResource : RenderCommand
	{
		RenderCommand_UnsetTextureResource() { type = RenderCommandType::UnsetTextureResource; }

		U32 textureSlot;
	};

	struct RenderCommand_Draw : RenderCommand
	{
		RenderCommand_Draw() { type = RenderCommandType::Draw; }

		VertexBufferHandle vertexBuffer;
		size_t count;
	};

	struct RenderCommand_DrawIndexed : RenderCommand
	{
		RenderCommand_DrawIndexed() { type = RenderCommandType::DrawIndexed; }

		IndexBufferHandle indexBuffer;
	};

	struct RenderCommand_DrawFullScreenQuad : RenderCommand
	{
		RenderCommand_DrawFullScreenQuad() { type = RenderCommandType::DrawFullScreenQuad; }
	};
}