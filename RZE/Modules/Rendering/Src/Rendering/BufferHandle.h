#pragma once

#include <memory>

namespace Rendering
{
	class IVertexBuffer;
	class IIndexBuffer;
	class IConstantBuffer;

	class VertexBufferHandle
	{
		friend class Renderer;

	public:
		VertexBufferHandle() = default;
		virtual ~VertexBufferHandle() = default;

	public:
		bool IsValid() { return m_buffer != nullptr; }

	private:
		VertexBufferHandle(const std::shared_ptr<IVertexBuffer>& buffer)
			: m_buffer(buffer) {}

		std::shared_ptr<IVertexBuffer> m_buffer;
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
		IndexBufferHandle(const std::shared_ptr<IIndexBuffer>& buffer)
			: m_buffer(buffer) {}

		std::shared_ptr<IIndexBuffer> m_buffer;
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
		ConstantBufferHandle(const std::shared_ptr<IConstantBuffer>& buffer)
			: m_buffer(buffer) {}

		std::shared_ptr<IConstantBuffer> m_buffer;
	};
}