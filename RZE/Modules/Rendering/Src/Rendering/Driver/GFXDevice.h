#pragma once

#include <Utils/PrimitiveDefs.h>
#include <Utils/Platform/FilePath.h>

#include <memory>
#include <vector>

namespace Rendering
{
	class IVertexBuffer;
	class IIndexBuffer;
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
	};
}