#pragma once

#include <vector>

#include <Utils/PrimitiveDefs.h>

namespace Diotima
{
	class IGFXVertexBuffer
	{
	public:
		IGFXVertexBuffer() = default;
		virtual ~IGFXVertexBuffer() = default;

		virtual void Allocate(void* data, U32 numElements) = 0;
	};

	class IGFXIndexBuffer
	{
	public:
		IGFXIndexBuffer() = default;
		virtual ~IGFXIndexBuffer() = default;

		virtual void Allocate(void* data, U32 numElements) = 0;
	};

	class IGFXDepthStencilBuffer
	{
	public:
		IGFXDepthStencilBuffer() = default;
		virtual ~IGFXDepthStencilBuffer() = default;

		virtual void Allocate() = 0;
	};

	class IGFXConstantBuffer
	{
	public:
		IGFXConstantBuffer() = default;
		virtual ~IGFXConstantBuffer() = default;

		virtual void Allocate(void* data, U32 numElements) = 0;
	};
}