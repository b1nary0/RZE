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
		virtual void SetWindow(void* windowHandle) = 0;
		virtual void Shutdown() = 0;

		virtual U32 CreateVertexBuffer(void* data, U32 numElements) = 0;
	};
}