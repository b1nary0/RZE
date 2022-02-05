#pragma once

#include <memory>

namespace Rendering
{
	class IGPUBuffer;

	class VertexBufferHandle
	{
		friend class Renderer;

	public:
		VertexBufferHandle() = default;
		~VertexBufferHandle() = default;

	private:
		VertexBufferHandle(const std::shared_ptr<IGPUBuffer>& buffer)
		{
			m_buffer = buffer;
		}

		std::shared_ptr<IGPUBuffer> m_buffer;
	};

	class IndexBufferHandle
	{
		friend class Renderer;

	public:
		IndexBufferHandle() = default;
		~IndexBufferHandle() = default;

	private:
		IndexBufferHandle(const std::shared_ptr<IGPUBuffer>& buffer)
		{
			m_buffer = buffer;
		}

		std::shared_ptr<IGPUBuffer> m_buffer;
	};
}