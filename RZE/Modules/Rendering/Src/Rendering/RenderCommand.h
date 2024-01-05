#pragma once

#include <Utils/PrimitiveDefs.h>

#include <memory>

namespace Rendering
{
	class DX11VertexBuffer;
	class DX11IndexBuffer;
	class DX11ConstantBuffer;
	class DX11TextureBuffer2D;

	enum RenderCommandType
	{
		Unset = 0,
		CreateVertexBuffer = 100,
		CreateIndexBuffer,
		CreateConstantBuffer,
		CreateTextureBuffer2D
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
}