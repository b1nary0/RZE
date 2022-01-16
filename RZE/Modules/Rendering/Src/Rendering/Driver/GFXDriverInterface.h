#pragma once

#include <vector>

#include <Utils/PrimitiveDefs.h>

//
// GFXDriverInterface
//
// Each driver may have their individual ecosystem setups outside of devices, etc. 
// For example: D3D has the debug layer which needs to be initialized. This class
// aims to help wrap that for each driver API to avoid bespoke setup in the renderer.
// The driver interface will also act as a factory of sorts to create the device etc.
// Ideally this will also help obfuscate the header includes.
namespace Rendering
{
	class IGFXDevice;

	class IGFXDriverInterface
	{
	public:
		IGFXDriverInterface() = default;
		virtual ~IGFXDriverInterface() = default;

	public:
		virtual void Initialize() = 0;
		virtual void Present() = 0;
		virtual void Shutdown() = 0;

		virtual U32 CreateVertexBuffer(void* data, U32 numElements) = 0;
		virtual U32 CreateIndexBuffer(void* data, U32 numElements) = 0;
		virtual U32 CreateTextureBuffer2D(void* data, U32 width, U32 height) = 0;
		virtual U32 CreateConstantBuffer(void* data, U32 size) = 0;

		virtual void SetWindow(void* windowHandle) = 0;
	};
}