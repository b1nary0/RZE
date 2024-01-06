#pragma once

#include <Rendering/BufferHandle.h>
#include <Rendering/Driver/ShaderTypes.h>
#include <Rendering/ShaderHandle.h>

#include <Utils/PrimitiveDefs.h>
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

	enum RenderCommandType
	{
		Unset = 0,
		CreateVertexBuffer = 100,
		CreateIndexBuffer,
		CreateConstantBuffer,
		CreateTextureBuffer2D,
		CreateVertexShader,
		CreatePixelShader,
		UploadDataToBuffer,
		ReleaseVertexShader,
		ReleasePixelShader,
		ClearDepthStencilBuffer
	};

	// @todo a dummy class for storing these all separately just to prove out system.
	// doubt it needs to be inherited just for storage but i dont wanna think about it atm
	struct RenderCommand
	{
		RenderCommandType type;
		
		virtual void Execute() = 0;
	};

	struct RenderCommand_CreateVertexBuffer : RenderCommand
	{
		RenderCommand_CreateVertexBuffer() { type = RenderCommandType::CreateVertexBuffer; }

		size_t dataTypeSize = 0;
		U32 stride = 0;
		size_t count = 0;
		void* data = nullptr;
		std::shared_ptr<DX11VertexBuffer> bufferPtr = nullptr;

		void Execute() override;
	};

	struct RenderCommand_CreateIndexBuffer : RenderCommand
	{
		RenderCommand_CreateIndexBuffer() { type = RenderCommandType::CreateIndexBuffer; }

		size_t dataTypeSize = 0;
		size_t count = 0;
		void* data = nullptr;
		std::shared_ptr<DX11IndexBuffer> bufferPtr = nullptr;

		void Execute() override;
	};

	struct RenderCommand_CreateConstantBuffer : RenderCommand
	{
		RenderCommand_CreateConstantBuffer() { type = RenderCommandType::CreateConstantBuffer; }

		size_t dataTypeSize = 0;
		size_t count = 0;
		void* data = nullptr;
		std::shared_ptr<DX11ConstantBuffer> bufferPtr = nullptr;

		void Execute() override;
	};

	struct RenderCommand_CreateTextureBuffer2D : RenderCommand
	{
		RenderCommand_CreateTextureBuffer2D() { type = RenderCommandType::CreateIndexBuffer; }

		const void* data = nullptr;
		std::shared_ptr<DX11TextureBuffer2D> bufferPtr = nullptr;

		void Execute() override;
	};

	struct RenderCommand_CreateVertexShader : RenderCommand
	{
		RenderCommand_CreateVertexShader() { type = RenderCommandType::CreateVertexShader; }

		Filepath filepath;
		ShaderInputLayout shaderInputLayout;
		std::shared_ptr<DX11VertexShader> bufferPtr = nullptr;

		void Execute() override;
	};

	struct RenderCommand_CreatePixelShader : RenderCommand
	{
		RenderCommand_CreatePixelShader() { type = RenderCommandType::CreatePixelShader; }

		Filepath filepath;
		std::shared_ptr<DX11PixelShader> bufferPtr = nullptr;

		void Execute() override;
	};

	struct RenderCommand_UploadDataToBuffer : RenderCommand
	{
		RenderCommand_UploadDataToBuffer() { type = RenderCommandType::UploadDataToBuffer; }

		const void* data = nullptr;
		ConstantBufferHandle bufferHandle;

		void Execute() override;
	};

	struct RenderCommand_ReleaseVertexShader : RenderCommand
	{
		RenderCommand_ReleaseVertexShader() { type = RenderCommandType::ReleaseVertexShader; }

		VertexShaderHandle shaderHandle;

		void Execute() override;
	};

	struct RenderCommand_ReleasePixelShader : RenderCommand
	{
		RenderCommand_ReleasePixelShader() { type = RenderCommandType::ReleasePixelShader; }

		PixelShaderHandle shaderHandle;

		void Execute() override;
	};

	struct RenderCommand_ClearDepthStencilBuffer : RenderCommand
	{
		RenderCommand_ClearDepthStencilBuffer() { type = RenderCommandType::ClearDepthStencilBuffer; }

		TextureBuffer2DHandle bufferHandle;

		void Execute() override;
	};
}