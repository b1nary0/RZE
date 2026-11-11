#pragma once

#include <Utils/PrimitiveDefs.h>

#include <cstddef>

namespace Rendering
{
	// Colour texture formats. RGBA8_UNORM is 0 so zero-initialized params keep the default.
	enum class ETextureFormat : U32
	{
		RGBA8_UNORM = 0,
		RGBA16_FLOAT,
		RG16_FLOAT,
		// Integer IDs, e.g. for picking; can't be filtered or have mips generated
		R32_UINT,
		// A 64-bit integer ID as two halves, low then high; otherwise like R32_UINT
		RG32_UINT,
		// Single channels, e.g. ambient occlusion (R8) or linear depth (R16, R32)
		R8_UNORM,
		R16_FLOAT,
		R32_FLOAT,
		// Two 8-bit channels, e.g. a lookup table
		RG8_UNORM
	};

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
		// Ignored for depth textures
		ETextureFormat Format;
		// How many slices: 0 or 1 for a single texture. A cube map has 6 per cube, in D3D's face order (+X, -X, +Y, -Y,
		// +Z, -Z; Engine's Graphics/CubeMap.h names them), so more than 6 makes a cube array. Arrays can't be multisampled
		// or made with texels.
		U32 ArraySize;
		//U32 Usage;
		bool bIsShaderResource;
		bool bIsRenderTarget;
		// Compute shaders can write it, one mip at a time (Renderer::SetUnorderedAccess). Not for depth textures.
		bool bIsUnorderedAccess;
		bool bIsDepthTexture;
		// Samples as a TextureCube (or TextureCubeArray) rather than a Texture2D(Array)
		bool bIsCubeMap;
	};

	class IVertexBuffer
	{
	public:
		IVertexBuffer() = default;
		virtual ~IVertexBuffer() = default;

		// Must be called before Allocate(). Dynamic buffers are created empty and written via UpdateData().
		virtual void SetDynamic(bool isDynamic) = 0;
		virtual void Allocate(const void* data, size_t size, U32 count, U32 stride) = 0;
		virtual void Release() = 0;

		// Overwrites the buffer's contents. Dynamic buffers only.
		virtual void UpdateData(const void* data, size_t bytes) = 0;

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

		// Overwrites the whole buffer with data, which is as big as it
		virtual void UpdateData(const void* data) = 0;

		virtual void SetActiveVS(U32 bufferSlot) = 0;
		virtual void SetActivePS(U32 bufferSlot) = 0;
		virtual void SetActiveCS(U32 bufferSlot) = 0;

		virtual U32 GetAlignment() { return 0; }
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

		virtual void Allocate(const void* data, const GFXTextureBufferParams& params) = 0;
		virtual void Release() = 0;

		virtual void SetActive(U32 textureSlot) = 0;
		// As SetActive, for compute shaders
		virtual void SetActiveCS(U32 textureSlot) = 0;

		virtual void* GetTextureData() = 0;

		virtual bool IsValid() = 0;
	};
}