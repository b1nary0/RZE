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
		virtual ~VertexBufferHandle() = default;

	public:
		bool IsValid() { return m_buffer != nullptr; }

	private:
		VertexBufferHandle(const std::shared_ptr<IGPUBuffer>& buffer)
			: m_buffer(buffer) {}

		std::shared_ptr<IGPUBuffer> m_buffer;
	};

	class IndexBufferHandle
	{
		friend class Renderer;

	public:
		IndexBufferHandle() = default;
		virtual ~IndexBufferHandle() = default;

	public:
		bool IsValid() { return m_buffer != nullptr; }

	private:
		IndexBufferHandle(const std::shared_ptr<IGPUBuffer>& buffer)
			: m_buffer(buffer) {}

		std::shared_ptr<IGPUBuffer> m_buffer;
	};

	class ConstantBufferHandle
	{
		friend class Renderer;

	public:
		ConstantBufferHandle() = default;
		virtual ~ConstantBufferHandle() = default;

	public:
		bool IsValid() { return m_buffer != nullptr; }

	private:
		ConstantBufferHandle(const std::shared_ptr<IGPUBuffer>& buffer)
			: m_buffer(buffer) {}

		std::shared_ptr<IGPUBuffer> m_buffer;
	};
}