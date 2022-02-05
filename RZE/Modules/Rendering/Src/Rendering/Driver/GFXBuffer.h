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

	class IGPUBuffer
	{
	public:
		IGPUBuffer() = default;
		virtual ~IGPUBuffer() = default;

		virtual void Allocate(void* data, size_t size, U32 count) = 0;
		virtual void Release() = 0;

		virtual void SetActive() = 0;
	};
	
	class IGFXDepthStencilBuffer
	{
	public:
		IGFXDepthStencilBuffer() = default;
		virtual ~IGFXDepthStencilBuffer() = default;

		virtual void Allocate() = 0;
		virtual void Release() = 0;
	};

	class IGFXConstantBuffer
	{
	public:
		IGFXConstantBuffer() = default;
		virtual ~IGFXConstantBuffer() = default;

		virtual void Allocate(size_t size, U32 count) = 0;
		virtual void Release() = 0;
	};

	class IGFXTextureBuffer2D
	{
	public:
		IGFXTextureBuffer2D() = default;
		virtual ~IGFXTextureBuffer2D() = default;

		virtual void Allocate(void* data, const GFXTextureBufferParams& params) = 0;
		virtual void Release() = 0;
	};
}