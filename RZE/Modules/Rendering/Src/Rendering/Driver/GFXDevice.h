#pragma once

#include <Utils/PrimitiveDefs.h>
#include <Utils/Platform/FilePath.h>

#include <memory>
#include <vector>

namespace Rendering
{
	class IGFXVertexBuffer;
	class IGFXIndexBuffer;
	class IGFXConstantBuffer;

	struct GFXTextureBufferParams;

	enum class EGFXBufferType
	{
		Vertex,
		Index,
		Texture2D
	};

	class IGFXDevice
	{
	public:
		IGFXDevice() = default;
		virtual ~IGFXDevice() = default;

	public:
		virtual void Initialize() = 0;
		virtual void Present() = 0;
		virtual void SetWindow(void* windowHandle) = 0;
		virtual void Shutdown() = 0;

		virtual IGFXVertexBuffer* CreateVertexBuffer(void* data, size_t size, U32 count) = 0;
		virtual IGFXIndexBuffer* CreateIndexBuffer(void* data, size_t size, U32 count) = 0;
		virtual IGFXConstantBuffer* CreateConstantBuffer(size_t memberSize, U32 maxMembers) = 0;
		virtual Int32 CreateTextureBuffer2D(void* data, const GFXTextureBufferParams& params) = 0;

		virtual Int32 CreatePixelShader(const FilePath& filePath) = 0;
	};
}