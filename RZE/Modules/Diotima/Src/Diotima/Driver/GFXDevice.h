#pragma once

#include <vector>

#include <Utils/PrimitiveDefs.h>

namespace Diotima
{
	class IGFXVertexBuffer;

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

		virtual U32 CreateVertexBuffer(void* data, U32 numElements) = 0;
		virtual U32 CreateIndexBuffer(void* data, U32 numElements) = 0;
		virtual U32 CreateTextureBuffer2D(void* data, U32 width, U32 height) = 0;
		virtual U32 CreateConstantBuffer(size_t memberSize, U32 maxMembers) = 0;
	};
}