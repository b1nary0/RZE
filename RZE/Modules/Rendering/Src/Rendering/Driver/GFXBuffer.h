#pragma once

#include <Utils/PrimitiveDefs.h>

namespace Rendering
{
	// This isn't the complete answer to this atm, but will do for now
	// to solve the problems at hand.. I fear I am leaving things
	// too late, but what needs to get done will get done.
	struct GFXTextureBufferParams
	{
		U32 Width;
		U32 Height;
		U32 SampleCount;
		U32 SampleQuality;
		U32 MipLevels;
		U32 MostDetailedMip;
		//U32 Format;
		//U32 Usage;
		bool bIsShaderResource;
		bool bIsRenderTarget;
		bool bIsDepthTexture;
	};

	class IVertexBuffer
	{
	public:
		IVertexBuffer() = default;
		virtual ~IVertexBuffer() = default;

		virtual void Allocate(const void* data, size_t size, U32 count) = 0;
		virtual void Release() = 0;

		// @TODO Index buffers and vertex buffers don't fit this interface very well
		// Will have to go back to previous implementation of interface for each
		virtual void SetActive(U32 bufferSlot) = 0;
	};

	class IIndexBuffer
	{
	public:
		IIndexBuffer() = default;
		virtual ~IIndexBuffer() = default;

		virtual void Allocate(const void* data, size_t size, U32 count) = 0;
		virtual void Release() = 0;

		// @TODO Index buffers and vertex buffers don't fit this interface very well
		// Will have to go back to previous implementation of interface for each
		virtual void SetActive() = 0;

		virtual U32 GetIndexCount() = 0;
	};

	class IConstantBuffer
	{
	public:
		IConstantBuffer() = default;
		virtual ~IConstantBuffer() = default;

		virtual void Allocate(const void* data, size_t size, U32 count) = 0;
		virtual void Release() = 0;

		virtual void SetActiveVS(U32 bufferSlot) = 0;
		virtual void SetActivePS(U32 bufferSlot) = 0;
	};

	class IGFXDepthStencilBuffer
	{
	public:
		IGFXDepthStencilBuffer() = default;
		virtual ~IGFXDepthStencilBuffer() = default;

		virtual void Allocate() = 0;
		virtual void Release() = 0;
	};

	class ITextureBuffer2D
	{
	public:
		ITextureBuffer2D() = default;
		virtual ~ITextureBuffer2D() = default;

		virtual void Allocate(void* data, const GFXTextureBufferParams& params) = 0;
		virtual void Release() = 0;

		virtual void SetActive(U32 textureSlot) = 0;
	};
}