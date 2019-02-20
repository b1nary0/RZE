#pragma once

#include <vector>

namespace Diotima
{
	class IGFXVertexBuffer
	{
	public:
		IGFXVertexBuffer() = default;
		virtual ~IGFXVertexBuffer() = default;

		virtual void Allocate(const std::vector<float>& data) = 0;
	};
}