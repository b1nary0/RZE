#pragma once

#include <memory>

namespace Rendering
{
	class IConstantBuffer;
	class IIndexBuffer;
	class IVertexBuffer;
	class ITextureBuffer2D;


	class VertexBufferHandle
	{
		friend class Renderer;

	public:
		VertexBufferHandle() = default;
		~VertexBufferHandle() = default;

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
		~IndexBufferHandle() = default;

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
		~ConstantBufferHandle() = default;

	public:
		bool IsValid() { return m_buffer != nullptr; }
		
	private:
		ConstantBufferHandle(const std::shared_ptr<IConstantBuffer>& buffer)
			: m_buffer(buffer) {}

		std::shared_ptr<IConstantBuffer> m_buffer;
	};

	class TextureBuffer2DHandle
	{
		friend class Renderer;

	public:
		TextureBuffer2DHandle() = default;
		~TextureBuffer2DHandle() = default;

	public:
		bool IsValid() { return m_buffer != nullptr; }

		void* GetTextureData() const;

	private:
		TextureBuffer2DHandle(const std::shared_ptr<ITextureBuffer2D>& buffer)
			: m_buffer(buffer) {}

		std::shared_ptr<ITextureBuffer2D> m_buffer;
	};

	using RenderTargetHandle = TextureBuffer2DHandle;
}